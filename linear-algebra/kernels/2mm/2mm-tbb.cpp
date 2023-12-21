#include <chrono>
#include <iomanip>
#include <iostream>

#include <tbb/tbb.h>

#include <noarr/structures_extended.hpp>
#include <noarr/structures/interop/bag.hpp>
#include <noarr/structures/interop/serialize_data.hpp>
#include <noarr/structures/interop/tbb.hpp>

#include "defines.hpp"
#include "2mm.hpp"

using num_t = DATA_TYPE;

namespace {

constexpr auto i_vec =  noarr::vector<'i'>();
constexpr auto j_vec =  noarr::vector<'j'>();
constexpr auto k_vec =  noarr::vector<'k'>();
constexpr auto l_vec =  noarr::vector<'l'>();

struct tuning {
	DEFINE_PROTO_STRUCT(tmp_layout, j_vec ^ i_vec);
	DEFINE_PROTO_STRUCT(a_layout, k_vec ^ i_vec);
	DEFINE_PROTO_STRUCT(b_layout, j_vec ^ k_vec);
	DEFINE_PROTO_STRUCT(c_layout, l_vec ^ j_vec);
	DEFINE_PROTO_STRUCT(d_layout, l_vec ^ i_vec);
} tuning;

// initialization function
void init_array(num_t &alpha, num_t &beta, auto A, auto B, auto C, auto D) noexcept {
	// tmp: i x j
	// A: i x k
	// B: k x j
	// C: j x l
	// D: i x l

	alpha = (num_t)1.5;
	beta = (num_t)1.2;

	auto ni = A | noarr::get_length<'i'>();
	auto nj = B | noarr::get_length<'j'>();
	auto nk = A | noarr::get_length<'k'>();
	auto nl = C | noarr::get_length<'l'>();

	noarr::traverser(A)
		.for_each([=](auto state) {
			auto [i, k] = noarr::get_indices<'i', 'k'>(state);
			A[state] = (num_t)((i * k + 1) % ni) / ni;
		});

	noarr::traverser(B)
		.for_each([=](auto state) {
			auto [k, j] = noarr::get_indices<'k', 'j'>(state);
			B[state] = (num_t)(k * (j + 1) % nj) / nj;
		});

	noarr::traverser(C)
		.for_each([=](auto state) {
			auto [j, l] = noarr::get_indices<'j', 'l'>(state);
			C[state] = (num_t)((j * (l + 3) + 1) % nl) / nl;
		});

	noarr::traverser(D)
		.for_each([=](auto state) {
			auto [i, l] = noarr::get_indices<'i', 'l'>(state);
			D[state] = (num_t)(i * (l + 2) % nk) / nk;
		});
}

// computation kernel
[[gnu::flatten, gnu::noinline]]
void kernel_2mm(num_t alpha, num_t beta, auto tmp, auto A, auto B, auto C, auto D) noexcept {
	// tmp: i x j
	// A: i x k
	// B: k x j
	// C: j x l
	// D: i x l

	auto trav1 = noarr::traverser(tmp, A, B);
	noarr::tbb_for_each(
		trav1.order(noarr::reorder<'i'>()),
		[=](auto state) {
			auto inner = trav1.order(noarr::fix(state));

			
			inner.template for_dims<'j'>([=](auto inner) {
				auto state = inner.state();

				tmp[state] = 0;

				inner.for_each([=](auto state) {
					tmp[state] += alpha * A[state] * B[state];
				});
			});
		});

	auto trav2 = noarr::traverser(D, tmp, C);
	noarr::tbb_for_each(
		trav2.order(noarr::reorder<'i'>()),
		[=](auto state) {
			auto inner = trav2.order(noarr::fix(state));

			
			inner.template for_dims<'l'>([=](auto inner) {
				auto state = inner.state();

				D[state] *= beta;

				inner.for_each([=](auto state) {
					D[state] += tmp[state] * C[state];
				});
			});
		});
}

} // namespace

int main(int argc, char *argv[]) {
	using namespace std::string_literals;

	// problem size
	std::size_t ni = NI;
	std::size_t nj = NJ;
	std::size_t nk = NK;
	std::size_t nl = NL;

	// data
	num_t alpha;
	num_t beta;

	auto set_lengths = noarr::set_length<'i'>(ni) ^ noarr::set_length<'j'>(nj) ^ noarr::set_length<'k'>(nk) ^ noarr::set_length<'l'>(nl);

	auto tmp = noarr::make_bag(noarr::scalar<num_t>() ^ tuning.tmp_layout ^ set_lengths);

	auto A = noarr::make_bag(noarr::scalar<num_t>() ^ tuning.a_layout ^ set_lengths);
	auto B = noarr::make_bag(noarr::scalar<num_t>() ^ tuning.b_layout ^ set_lengths);
	auto C = noarr::make_bag(noarr::scalar<num_t>() ^ tuning.c_layout ^ set_lengths);

	auto D = noarr::make_bag(noarr::scalar<num_t>() ^ tuning.d_layout ^ set_lengths);

	// initialize data
	init_array(alpha, beta, A.get_ref(), B.get_ref(), C.get_ref(), D.get_ref());

	auto start = std::chrono::high_resolution_clock::now();

	// run kernel
	kernel_2mm(alpha, beta, tmp.get_ref(), A.get_ref(), B.get_ref(), C.get_ref(), D.get_ref());

	auto end = std::chrono::high_resolution_clock::now();

	auto duration = std::chrono::duration<long double>(end - start);

	// print results
	if (argc > 0 && argv[0] != ""s) {
		std::cout << std::fixed << std::setprecision(2);
		noarr::serialize_data(std::cout, D.get_ref() ^ noarr::hoist<'i'>());
	}

	std::cerr << duration.count() << std::endl;
}
