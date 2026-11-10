#include "game/hsd_jobj.h"
#include "game/ut.h"

extern "C" const float sGeometryZero;

/**
 * @note Address: 0x8018E640
 * @note Size: 0x1E4
 */
extern "C" int IntersectLinesXZ(const Vec* a, const Vec* b, const Vec* c, const Vec* d, Vec* out, int check)
{
	f32 abx   = b->x - a->x;
	f32 abz   = b->z - a->z;
	f32 acx   = c->x - a->x;
	f32 acz   = c->z - a->z;
	f32 sideC = abx * acz - abz * acx;
	f32 sideD = abx * (d->z - a->z) - abz * (d->x - a->x);
	f32 sideA = (d->x - c->x) * (a->z - c->z) - (d->z - c->z) * (a->x - c->x);
	f32 sideB = (d->x - c->x) * (b->z - c->z) - (d->z - c->z) * (b->x - c->x);

	int hit;
	if (check == 1) {
		hit = sideC * sideD <= sGeometryZero && sideA * sideB <= sGeometryZero;
	} else {
		hit = true;
	}
	if (hit) {
		if (b->x == a->x) {
			out->x = a->x;
			out->z = c->z + (a->x - c->x) * ((d->z - c->z) / (d->x - c->x));
		} else if (d->x == c->x) {
			out->x = c->x;
			out->z = a->z + (c->x - a->x) * ((b->z - a->z) / (b->x - a->x));
		} else {
			f32 slopeAB = abz / abx;
			f32 slopeCD = (d->z - c->z) / (d->x - c->x);
			if (slopeAB == slopeCD) {
				return 0;
			}
			f32 t  = (acz - slopeCD * acx) / (slopeAB - slopeCD);
			f32 dz = t * slopeAB;
			out->x = t + a->x;
			out->z = dz + a->z;
		}
		out->y = sGeometryZero;
		return 1;
	}
	return 0;
}
/*
    stwu     r1, -0x60(r1)
    stfd     f31, 0x50(r1)
    psq_st   f31, 0x58(r1), 0, qr0
    stfd     f30, 0x40(r1)
    psq_st   f30, 0x48(r1), 0, qr0
    stfd     f29, 0x30(r1)
    psq_st   f29, 0x38(r1), 0, qr0
    stfd     f28, 0x20(r1)
    psq_st   f28, 0x28(r1), 0, qr0
    stfd     f27, 0x10(r1)
    psq_st   f27, 0x18(r1), 0, qr0
    lfs      f10, 0x0(r4)
    cmpwi    r8, 0x1
    lfs      f6, 0x0(r3)
    lfs      f7, 0x0(r5)
    lfs      f8, 0x0(r6)
    fsubs    f12, f10, f6
    lfs      f11, 0x8(r4)
    fsubs    f29, f7, f6
    lfs      f2, 0x8(r3)
    fsubs    f4, f8, f6
    lfs      f9, 0x8(r5)
    lfs      f3, 0x8(r6)
    fsubs    f13, f11, f2
    fsubs    f1, f6, f7
    fsubs    f30, f3, f9
    fsubs    f0, f10, f7
    fsubs    f28, f9, f2
    fmuls    f5, f13, f29
    fsubs    f27, f3, f2
    fmuls    f4, f13, f4
    fsubs    f31, f8, f7
    fsubs    f3, f2, f9
    fmuls    f2, f30, f1
    fsubs    f1, f11, f9
    fmuls    f0, f30, f0
    fmsubs   f5, f12, f28, f5
    fmsubs   f4, f12, f27, f4
    fmsubs   f2, f31, f3, f2
    fmsubs   f3, f31, f1, f0
    bne      .L_8018E718
    fmuls    f0, f5, f4
    lfs      f1, sGeometryZero@sda21(r0)
    li       r0, 0x0
    fcmpo    cr0, f0, f1
    cror     eq, lt, eq
    bne      .L_8018E710
    fmuls    f0, f2, f3
    fcmpo    cr0, f0, f1
    cror     eq, lt, eq
    bne      .L_8018E710
    li       r0, 0x1
.L_8018E710:
    clrlwi   r0, r0, 24
    b        .L_8018E71C
.L_8018E718:
    li       r0, 0x1
.L_8018E71C:
    cmpwi    r0, 0x0
    beq      .L_8018E7F0
    fcmpu    cr0, f10, f6
    bne      .L_8018E764
    lfs      f4, 0x8(r5)
    lfs      f1, 0x8(r6)
    lfs      f3, 0x0(r5)
    lfs      f0, 0x0(r6)
    fsubs    f1, f1, f4
    lfs      f2, 0x0(r3)
    fsubs    f0, f0, f3
    stfs     f6, 0x0(r7)
    fsubs    f2, f2, f3
    fdivs    f0, f1, f0
    fmuls    f0, f2, f0
    fadds    f0, f4, f0
    stfs     f0, 0x8(r7)
    b        .L_8018E7E0
.L_8018E764:
    fcmpu    cr0, f8, f7
    bne      .L_8018E7A4
    lfs      f4, 0x8(r3)
    lfs      f1, 0x8(r4)
    lfs      f3, 0x0(r3)
    lfs      f0, 0x0(r4)
    fsubs    f1, f1, f4
    lfs      f2, 0x0(r5)
    fsubs    f0, f0, f3
    stfs     f7, 0x0(r7)
    fsubs    f2, f2, f3
    fdivs    f0, f1, f0
    fmuls    f0, f2, f0
    fadds    f0, f4, f0
    stfs     f0, 0x8(r7)
    b        .L_8018E7E0
.L_8018E7A4:
    fdivs    f3, f13, f12
    fdivs    f1, f30, f31
    fcmpu    cr0, f3, f1
    bne      .L_8018E7BC
    li       r3, 0x0
    b        .L_8018E7F4
.L_8018E7BC:
    fnmsubs  f2, f1, f29, f28
    lfs      f0, 0x8(r3)
    fsubs    f1, f3, f1
    fdivs    f2, f2, f1
    fmuls    f1, f2, f3
    fadds    f2, f2, f6
    fadds    f0, f1, f0
    stfs     f2, 0x0(r7)
    stfs     f0, 0x8(r7)
.L_8018E7E0:
    lfs      f0, sGeometryZero@sda21(r0)
    li       r3, 0x1
    stfs     f0, 0x4(r7)
    b        .L_8018E7F4
.L_8018E7F0:
    li       r3, 0x0
.L_8018E7F4:
    psq_l    f31, 0x58(r1), 0, qr0
    lfd      f31, 0x50(r1)
    psq_l    f30, 0x48(r1), 0, qr0
    lfd      f30, 0x40(r1)
    psq_l    f29, 0x38(r1), 0, qr0
    lfd      f29, 0x30(r1)
    psq_l    f28, 0x28(r1), 0, qr0
    lfd      f28, 0x20(r1)
    psq_l    f27, 0x18(r1), 0, qr0
    lfd      f27, 0x10(r1)
    addi     r1, r1, 0x60
    blr
*/
