#include "noarr/structures_extended.hpp"
#include "noarr/structures/extra/traverser.hpp"

using num_t = float;

namespace {

// initialization function
void init_array(auto u) {
    // u: i x j

    auto n = u | noarr::get_length<'i'>();

    noarr::traverser(u)
        .for_each([=](auto state) {
            auto [i, j] = state | noarr::get_indices<'i', 'j'>(state);

            u[state] = (num_t)(i + n - j) / n;
        });
}

// computation kernel
void kernel_adi(auto steps, auto u, auto v, auto p, auto q) {
    // u: i x j
    // v: j x i
    // p: i x j
    // q: i x j

    auto u_trans = u ^ noarr::rename<'i', 'j', 'j', 'i'>();
    auto v_trans = v ^ noarr::rename<'i', 'j', 'j', 'i'>();
    auto traverser = noarr::traverser(u, v, p, q).order(noarr::bcast<'t'>(steps));

    num_t DX = (num_t)1.0 / (traverser | noarr::get_length<'i'>());
    num_t DY = (num_t)1.0 / (traverser | noarr::get_length<'j'>());
    num_t DT = (num_t)1.0 / (traverser | noarr::get_length<'t'>());

    num_t B1 = 2.0;
    num_t B2 = 1.0;

    num_t mul1 = B1 * DT / (DX * DX);
    num_t mul2 = B2 * DT / (DY * DY);

    num_t a = -mul1 / (num_t)2.0;
    num_t b = (num_t)1.0 + mul1;
    num_t c = a;

    num_t d = -mul2 / (num_t)2.0;
    num_t e = (num_t)1.0 + mul2;
    num_t f = d;

    traverser.order(noarr::symmetric_spans<'i', 'j'>(traverser.top_struct(), 1, 1))
        .template for_dims<'t'>([=](auto inner) {
            // column sweep
            inner.template for_dims<'i'>([=](auto inner) {
                auto state = inner.state();

                v[state & noarr::idx<'j'>(0)] = (num_t)1.0;
                p[state & noarr::idx<'j'>(0)] = (num_t)0.0;
                q[state & noarr::idx<'j'>(0)] = v[state & noarr::idx<'j'>(0)];

                inner.template for_each<'j'>([=](auto state) {
                    p[state] = -c / (b + a * p[noarr::neighbor<'j'>(state, -1)]);
                    q[state] = (-d * u_trans[noarr::neighbor<'i'>(state, -1)] + (B1 + B2 * d) * u_trans[state] -
                                 f * u_trans[noarr::neighbor<'i'>(state, +1)] - a * q[noarr::neighbor<'j'>(state, -1)]) /
                               (b + a * p[noarr::neighbor<'j'>(state, -1)]);
                });

                v[state & (traverser | noarr::get_length<'j'>()) - 1] = (num_t)1.0;

                inner
                    .order(noarr::reverse<'j'>())
                    .template for_each<'j'>([=](auto state) {
                        v[state] = p[state] * v[noarr::neighbor<'j'>(state, 1)] + q[state];
                    });
            });

            // row sweep
            inner.template for_dims<'i'>([=](auto inner) {
                auto state = inner.state();

                u[state & noarr::idx<'j'>(0)] = (num_t)1.0;
                p[state & noarr::idx<'j'>(0)] = (num_t)0.0;
                q[state & noarr::idx<'j'>(0)] = u[state & noarr::idx<'j'>(0)];

                inner.template for_each<'j'>([=](auto state) {
                    p[state] = -f / (e + d * p[noarr::neighbor<'j'>(state, -1)]);
                    q[state] = (-a * v_trans[noarr::neighbor<'i'>(state, -1)] + (B1 + B2 * a) * v_trans[state] -
                                 c * v_trans[noarr::neighbor<'i'>(state, +1)] - d * q[noarr::neighbor<'j'>(state, -1)]) /
                               (e + d * p[noarr::neighbor<'j'>(state, -1)]);
                });

                u[state & (traverser | noarr::get_length<'j'>()) - 1] = (num_t)1.0;

                inner
                    .order(noarr::reverse<'j'>())
                    .template for_each<'j'>([=](auto state) {
                        u[state] = p[state] * u[noarr::neighbor<'j'>(state, 1)] + q[state];
                    });
            });
        });
}

} // namespace

int main() { /* placeholder */}
