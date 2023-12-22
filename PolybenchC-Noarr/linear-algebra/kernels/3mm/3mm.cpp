#include <chrono>
#include <iomanip>
#include <iostream>

#include <noarr/structures_extended.hpp>
#include <noarr/structures/extra/planner.hpp>
#include <noarr/structures/interop/bag.hpp>
#include <noarr/structures/interop/serialize_data.hpp>

#include "defines.hpp"
#include "3mm.hpp"

using num_t = DATA_TYPE;

namespace {

constexpr auto i_vec =  noarr::vector<'i'>();
constexpr auto j_vec =  noarr::vector<'j'>();
constexpr auto k_vec =  noarr::vector<'k'>();
constexpr auto l_vec =  noarr::vector<'l'>();
constexpr auto m_vec =  noarr::vector<'m'>();

struct tuning {
	DEFINE_PROTO_STRUCT(block_i1, noarr::hoist<'i'>());
	DEFINE_PROTO_STRUCT(block_j1, noarr::hoist<'j'>());
	DEFINE_PROTO_STRUCT(block_j2, noarr::hoist<'j'>());
	DEFINE_PROTO_STRUCT(block_l2, noarr::hoist<'l'>());
	DEFINE_PROTO_STRUCT(block_i3, noarr::hoist<'i'>());
	DEFINE_PROTO_STRUCT(block_l3, noarr::hoist<'l'>());

	DEFINE_PROTO_STRUCT(order1, block_j1 ^ block_i1);
	DEFINE_PROTO_STRUCT(order2, block_l2 ^ block_j2);
	DEFINE_PROTO_STRUCT(order3, block_l3 ^ block_i3);

	DEFINE_PROTO_STRUCT(e_layout, j_vec ^ i_vec);
	DEFINE_PROTO_STRUCT(a_layout, k_vec ^ i_vec);
	DEFINE_PROTO_STRUCT(b_layout, j_vec ^ k_vec);
	DEFINE_PROTO_STRUCT(f_layout, l_vec ^ j_vec);
	DEFINE_PROTO_STRUCT(c_layout, m_vec ^ j_vec);
	DEFINE_PROTO_STRUCT(d_layout, l_vec ^ m_vec);
	DEFINE_PROTO_STRUCT(g_layout, l_vec ^ i_vec);
} tuning;

// initialization function
void init_array(auto A, auto B, auto C, auto D) noexcept {
	// A: i x k
	// B: k x j
	// C: j x m
	// D: m x l

	auto ni = A | noarr::get_length<'i'>();
	auto nj = B | noarr::get_length<'j'>();
	auto nk = A | noarr::get_length<'k'>();
	auto nl = D | noarr::get_length<'l'>();

	noarr::traverser(A)
		.for_each([=](auto state) {
			auto [i, k] = noarr::get_indices<'i', 'k'>(state);
			A[state] = (num_t)((i * k + 1) % ni) / (5 * ni);
		});

	noarr::traverser(B)
		.for_each([=](auto state) {
			auto [k, j] = noarr::get_indices<'k', 'j'>(state);
			B[state] = (num_t)((k * (j + 1) + 2) % nj) / (5 * nj);
		});

	noarr::traverser(C)
		.for_each([=](auto state) {
			auto [j, m] = noarr::get_indices<'j', 'm'>(state);
			C[state] = (num_t)(j * (m + 3) % nl) / (5 * nl);
		});

	noarr::traverser(D)
		.for_each([=](auto state) {
			auto [m, l] = noarr::get_indices<'m', 'l'>(state);
			D[state] = (num_t)((m * (l + 2) + 2) % nk) / (5 * nk);
		});
}

// computation kernel
template<class Order1 = noarr::neutral_proto, class Order2 = noarr::neutral_proto, class Order3 = noarr::neutral_proto>
[[gnu::flatten, gnu::noinline]]
void kernel_3mm(auto E, auto A, auto B, auto F, auto C, auto D, auto G, Order1 order1 = {}, Order2 order2 = {}, Order3 order3 = {}) noexcept {
	// E: i x j
	// A: i x k
	// B: k x j
	// F: j x l
	// C: j x m
	// D: m x l
	// G: i x l

	constexpr auto madd = [](auto &&m, auto &&l, auto &&r) {
		m += l * r;
	};

	#pragma scop
	noarr::planner(E, A, B)
		.for_each_elem(madd)
		.template for_sections<'i', 'j'>([=](auto inner) {
			E[inner.state()] = 0;
			inner();
		})
		.order(noarr::hoist<'k'>())
		.order(noarr::hoist<'j'>())
		.order(noarr::hoist<'i'>())
		.order(order1)
		();

	noarr::planner(F, C, D)
		.for_each_elem(madd)
		.template for_sections<'j', 'l'>([=](auto inner) {
			F[inner.state()] = 0;
			inner();
		})
		.order(noarr::hoist<'m'>())
		.order(noarr::hoist<'l'>())
		.order(noarr::hoist<'j'>())
		.order(order2)
		();

	noarr::planner(G, E, F)
		.for_each_elem(madd)
		.template for_sections<'i', 'l'>([=](auto inner) {
			G[inner.state()] = 0;
			inner();
		})
		.order(noarr::hoist<'j'>())
		.order(noarr::hoist<'l'>())
		.order(noarr::hoist<'i'>())
		.order(order3)
		();
	#pragma endscop
}

} // namespace

int main(int argc, char *argv[]) {
	using namespace std::string_literals;

	// problem size
	std::size_t ni = NI;
	std::size_t nj = NJ;
	std::size_t nk = NK;
	std::size_t nl = NL;
	std::size_t nm = NM;

	auto set_lengths = noarr::set_length<'i'>(ni) ^ noarr::set_length<'j'>(nj) ^ noarr::set_length<'k'>(nk) ^ noarr::set_length<'l'>(nl) ^ noarr::set_length<'m'>(nm);

	// data
	auto E = noarr::make_bag(noarr::scalar<num_t>() ^ tuning.e_layout ^ set_lengths);
	auto A = noarr::make_bag(noarr::scalar<num_t>() ^ tuning.a_layout ^ set_lengths);
	auto B = noarr::make_bag(noarr::scalar<num_t>() ^ tuning.b_layout ^ set_lengths);

	auto F = noarr::make_bag(noarr::scalar<num_t>() ^ tuning.f_layout ^ set_lengths);
	auto C = noarr::make_bag(noarr::scalar<num_t>() ^ tuning.c_layout ^ set_lengths);
	auto D = noarr::make_bag(noarr::scalar<num_t>() ^ tuning.d_layout ^ set_lengths);

	auto G = noarr::make_bag(noarr::scalar<num_t>() ^ tuning.g_layout ^ set_lengths);

	// initialize data
	init_array(A.get_ref(), B.get_ref(), C.get_ref(), D.get_ref());

	auto start = std::chrono::high_resolution_clock::now();

	// run kernel
	kernel_3mm(E.get_ref(), A.get_ref(), B.get_ref(),
		F.get_ref(), C.get_ref(), D.get_ref(),
		G.get_ref(),
		tuning.order1, tuning.order2, tuning.order3);

	auto end = std::chrono::high_resolution_clock::now();

	auto duration = std::chrono::duration<long double>(end - start);

	// print results
	if (argc > 0 && argv[0] != ""s) {
		std::cout << std::fixed << std::setprecision(2);
		noarr::serialize_data(std::cout, G.get_ref() ^ noarr::hoist<'i'>());
	}

	std::cerr << duration.count() << std::endl;
}