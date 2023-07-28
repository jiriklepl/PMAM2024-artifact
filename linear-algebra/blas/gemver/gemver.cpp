#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

#include <noarr/structures_extended.hpp>
#include <noarr/structures/extra/traverser.hpp>
#include <noarr/structures/interop/bag.hpp>
#include <noarr/structures/interop/serialize_data.hpp>

#include "defines.hpp"
#include "gemver.hpp"

using num_t = DATA_TYPE;

namespace {

// initialization function
void init_array(num_t &alpha, num_t &beta, auto A, auto u1, auto v1, auto u2, auto v2, auto w, auto x, auto y, auto z) {
	// A: i x j
	// u1: i
	// v1: j
	// u2: i
	// v2: j
	// w: i
	// x: i
	// y: j
	// z: i

	alpha = (num_t)1.5;
	beta = (num_t)1.2;

	auto v1_i = v1 ^ noarr::rename<'j', 'i'>();
	auto v2_i = v2 ^ noarr::rename<'j', 'i'>();
	auto y_i = y ^ noarr::rename<'j', 'i'>();

	num_t fn = A | noarr::get_length<'i'>();

	noarr::traverser(A, u1, u2, v1_i, v2_i, y_i, z, x, w)
		.template for_dims<'i'>([=](auto inner) {
			auto state = inner.state();


			auto i = noarr::get_index<'i'>(state);

			u1[state] = i;
			u2[state] = ((i + 1) / fn) / 2.0;
			v1_i[state] = ((i + 1) / fn) / 4.0;
			v2_i[state] = ((i + 1) / fn) / 6.0;
			y_i[state] = ((i + 1) / fn) / 8.0;
			z[state] = ((i + 1) / fn) / 9.0;
			x[state] = 0.0;
			w[state] = 0.0;

			inner.for_each([=](auto state) {
				auto j = noarr::get_index<'j'>(state);

				A[state] = (num_t)(j * i % (A | noarr::get_length<'i'>())) / (A | noarr::get_length<'i'>());
			});
		});
}

// computation kernel
void kernel_gemver(num_t alpha, num_t beta, auto A, auto u1, auto v1, auto u2, auto v2, auto w, auto x, auto y, auto z) {
	// A: i x j
	// u1: i
	// v1: j
	// u2: i
	// v2: j
	// w: i
	// x: i
	// y: j
	// z: i

	auto A_ji = A ^ noarr::rename<'i', 'j', 'j', 'i'>();
	auto x_j = x ^ noarr::rename<'i', 'j'>();

	noarr::traverser(A, u1, u2, v1, v2)
		.for_each([=](auto state) {
			A[state] = A[state] + u1[state] * v1[state] + u2[state] * v2[state];
		});

	noarr::traverser(x, A_ji, y)
		.for_each([=](auto state) {
			x[state] = x[state] + beta * A_ji[state] * y[state];
		});
	
	noarr::traverser(x, z)
		.for_each([=](auto state) {
			x[state] = x[state] + z[state];
		});

	noarr::traverser(A, w, x_j)
		.template for_each([=](auto state) {
		   w[state] = w[state] + alpha * A[state] * x_j[state];
		});
}

} // namespace

int main(int argc, char *argv[]) {
	using namespace std::string_literals;

	// problem size
	std::size_t n = N;

	// data
	num_t alpha;
	num_t beta;

	auto A = noarr::make_bag(noarr::scalar<num_t>() ^ noarr::sized_vectors<'i', 'j'>(n, n));
	auto u1 = noarr::make_bag(noarr::scalar<num_t>() ^ noarr::sized_vector<'i'>(n));
	auto v1 = noarr::make_bag(noarr::scalar<num_t>() ^ noarr::sized_vector<'j'>(n));
	auto u2 = noarr::make_bag(noarr::scalar<num_t>() ^ noarr::sized_vector<'i'>(n));
	auto v2 = noarr::make_bag(noarr::scalar<num_t>() ^ noarr::sized_vector<'j'>(n));
	auto w = noarr::make_bag(noarr::scalar<num_t>() ^ noarr::sized_vector<'i'>(n));
	auto x = noarr::make_bag(noarr::scalar<num_t>() ^ noarr::sized_vector<'i'>(n));
	auto y = noarr::make_bag(noarr::scalar<num_t>() ^ noarr::sized_vector<'j'>(n));
	auto z = noarr::make_bag(noarr::scalar<num_t>() ^ noarr::sized_vector<'i'>(n));

	// initialize data
	init_array(alpha, beta, A.get_ref(), u1.get_ref(), v1.get_ref(), u2.get_ref(), v2.get_ref(), w.get_ref(), x.get_ref(), y.get_ref(), z.get_ref());

	auto start = std::chrono::high_resolution_clock::now();

	// run kernel
	kernel_gemver(alpha, beta, A.get_ref(), u1.get_ref(), v1.get_ref(), u2.get_ref(), v2.get_ref(), w.get_ref(), x.get_ref(), y.get_ref(), z.get_ref());

	auto end = std::chrono::high_resolution_clock::now();

	auto duration = std::chrono::duration<long double>(end - start);

	// print results
	if (argc > 0 && argv[0] != ""s) {
		std::cout << std::fixed << std::setprecision(2);
		noarr::serialize_data(std::cout, w);
	}

	std::cerr << duration.count() << std::endl;
}
