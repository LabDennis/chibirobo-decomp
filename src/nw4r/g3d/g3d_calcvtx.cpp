#include <nw4r/g3d.h>
#include <nw4r/math.h>

namespace nw4r {
namespace g3d {

// NONMATCHING: retail uses paired-single assembly in the vertex blend loops
void CalcVtx(ResMdl mdl, AnmObjShp* pAnmShp, ResVtxPosData** ppVtxPosTable,
             ResVtxNrmData** ppVtxNrmTable, ResVtxClrData** ppVtxClrTable) {
    u32 numVtxPos = mdl.GetResVtxPosNumEntries();

    for (u32 id = 0; id < numVtxPos; id++) {
        if (!pAnmShp->TestExistence(id)) {
            continue;
        }

        ShpAnmResult result;
        const ShpAnmResult* pResult = pAnmShp->GetResult(&result, id);

        if (!(pResult->flags & ShpAnmResult::FLAG_ANM_EXISTS)) {
            continue;
        }

        if (pResult->flags & ShpAnmResult::FLAG_ANM_VTXPOS) {
            struct KeyShape {
                const math::VEC3* pVtx; // at 0x0
                f32 weight;             // at 0x4
            };

            ResVtxPos basePos = pResult->baseShapeVtxSet.resVtxPos;

            KeyShape keyShape[ShpAnmResult::MAX_KEY_SHAPE + 1];
            int numKeyShape = 0;

            ResVtxPos vtxPos(ppVtxPosTable[id]);
            math::VEC3* pVtxPosBuf = static_cast<math::VEC3*>(vtxPos.GetData());

            if (pResult->baseShapeWeight != 0.0f) {
                const math::VEC3* pBase;
                u8 stride;

                basePos.GetArray(reinterpret_cast<const void**>(&pBase),
                                 &stride);

                keyShape[numKeyShape].pVtx = pBase;
                keyShape[numKeyShape].weight = pResult->baseShapeWeight;
                numKeyShape++;
            }

            for (int i = 0; i < static_cast<int>(pResult->numKeyShape); i++) {
                if (pResult->keyShape[i].weight == 0.0f) {
                    continue;
                }

                ResVtxPos key(pResult->keyShape[i].vtxSet.resVtxPos);
                const void* pData = key.GetData();

                keyShape[numKeyShape].pVtx =
                    static_cast<const math::VEC3*>(pData);
                keyShape[numKeyShape].weight = pResult->keyShape[i].weight;
                numKeyShape++;
            }

            u32 numVtx = basePos.GetNumVtxPos();
            const math::VEC3* const pVtxPosBufEnd = pVtxPosBuf + numVtx;
            const KeyShape* const pKeyEnd = keyShape + numKeyShape;

            for (; pVtxPosBuf < pVtxPosBufEnd; pVtxPosBuf++) {
                KeyShape& rFirst = keyShape[0];
                f32 firstWeight = rFirst.weight;
                const math::VEC3* pVtx = rFirst.pVtx++;

                f32 x = pVtx->x * firstWeight;
                f32 y = pVtx->y * firstWeight;
                f32 z = pVtx->z * firstWeight;

                for (KeyShape* pKey = &keyShape[1]; pKey < pKeyEnd; pKey++) {
                    f32 keyWeight = pKey->weight;
                    const math::VEC3* pKeyVtx = pKey->pVtx++;

                    x += pKeyVtx->x * keyWeight;
                    y += pKeyVtx->y * keyWeight;
                    z += pKeyVtx->z * keyWeight;
                }

                pVtxPosBuf->x = x;
                pVtxPosBuf->y = y;
                pVtxPosBuf->z = z;
            }

            DC::StoreRange(vtxPos.GetData(), numVtx * sizeof(math::VEC3));
        }

        if ((pResult->flags & ShpAnmResult::FLAG_ANM_VTXNRM) &&
            ppVtxNrmTable != NULL) {

            struct KeyShape {
                const math::VEC3* pVtx; // at 0x0
                f32 weight;             // at 0x4
            };

            ResVtxNrm baseNrm = pResult->baseShapeVtxSet.resVtxNrm;

            KeyShape keyShape[ShpAnmResult::MAX_KEY_SHAPE + 1];
            int numKeyShape = 0;

            ResVtxNrm vtxNrm(ppVtxNrmTable[baseNrm.GetID()]);
            math::VEC3* pVtxNrmBuf = static_cast<math::VEC3*>(vtxNrm.GetData());

            if (pResult->baseShapeWeight != 0.0f) {
                const math::VEC3* pBase;
                u8 stride;

                baseNrm.GetArray(reinterpret_cast<const void**>(&pBase),
                                 &stride);

                keyShape[numKeyShape].pVtx = pBase;
                keyShape[numKeyShape].weight = pResult->baseShapeWeight;
                numKeyShape++;
            }

            for (int i = 0; i < static_cast<int>(pResult->numKeyShape); i++) {
                if (pResult->keyShape[i].weight == 0.0f) {
                    continue;
                }

                ResVtxNrm key(pResult->keyShape[i].vtxSet.resVtxNrm);
                const void* pData = key.GetData();

                keyShape[numKeyShape].pVtx =
                    static_cast<const math::VEC3*>(pData);
                keyShape[numKeyShape].weight = pResult->keyShape[i].weight;
                numKeyShape++;
            }

            u32 numVtx = baseNrm.GetNumVtxNrm();
            const math::VEC3* const pVtxNrmBufEnd = pVtxNrmBuf + numVtx;
            const KeyShape* const pKeyEnd = keyShape + numKeyShape;

            for (; pVtxNrmBuf < pVtxNrmBufEnd; pVtxNrmBuf++) {
                KeyShape& rFirst = keyShape[0];
                f32 firstWeight = rFirst.weight;
                const math::VEC3* pVtx = rFirst.pVtx++;

                f32 x = pVtx->x * firstWeight;
                f32 y = pVtx->y * firstWeight;
                f32 z = pVtx->z * firstWeight;

                for (KeyShape* pKey = &keyShape[1]; pKey < pKeyEnd; pKey++) {
                    f32 keyWeight = pKey->weight;
                    const math::VEC3* pKeyVtx = pKey->pVtx++;

                    x += pKeyVtx->x * keyWeight;
                    y += pKeyVtx->y * keyWeight;
                    z += pKeyVtx->z * keyWeight;
                }

                pVtxNrmBuf->x = x;
                pVtxNrmBuf->y = y;
                pVtxNrmBuf->z = z;
            }

            DC::StoreRange(vtxNrm.GetData(), numVtx * sizeof(math::VEC3));
        }

        if ((pResult->flags & ShpAnmResult::FLAG_ANM_VTXCLR) &&
            ppVtxClrTable != NULL) {

            struct KeyShape {
                const ut::Color* pVtx; // at 0x0
                f32 weight;            // at 0x4
            };

            ResVtxClr baseClr = pResult->baseShapeVtxSet.resVtxClr;

            KeyShape keyShape[ShpAnmResult::MAX_KEY_SHAPE + 1];
            int numKeyShape = 0;

            ResVtxClr vtxClr(ppVtxClrTable[baseClr.GetID()]);
            ut::Color* pVtxClrBuf = static_cast<ut::Color*>(vtxClr.GetData());

            if (pResult->baseShapeWeight != 0.0f) {
                const ut::Color* pBase;
                u8 stride;

                baseClr.GetArray(reinterpret_cast<const void**>(&pBase),
                                 &stride);

                keyShape[numKeyShape].pVtx = pBase;
                keyShape[numKeyShape].weight = pResult->baseShapeWeight;
                numKeyShape++;
            }

            for (int i = 0; i < static_cast<int>(pResult->numKeyShape); i++) {
                if (pResult->keyShape[i].weight == 0.0f) {
                    continue;
                }

                ResVtxClr key(pResult->keyShape[i].vtxSet.resVtxClr);
                const void* pData = key.GetData();

                keyShape[numKeyShape].pVtx =
                    static_cast<const ut::Color*>(pData);
                keyShape[numKeyShape].weight = pResult->keyShape[i].weight;
                numKeyShape++;
            }

            u32 numVtx = baseClr.GetNumVtxClr();
            const ut::Color* const pVtxClrBufEnd = pVtxClrBuf + numVtx;
            const KeyShape* const pKeyEnd = keyShape + numKeyShape;

            for (; pVtxClrBuf < pVtxClrBufEnd; pVtxClrBuf++) {
                KeyShape& rFirst = keyShape[0];
                f32 firstWeight = rFirst.weight;
                const ut::Color* pVtx = rFirst.pVtx++;

                f32 r = fastcast::U8_0ToF32(&pVtx->r) * firstWeight;
                f32 g = fastcast::U8_0ToF32(&pVtx->g) * firstWeight;
                f32 b = fastcast::U8_0ToF32(&pVtx->b) * firstWeight;
                f32 a = fastcast::U8_0ToF32(&pVtx->a) * firstWeight;

                for (KeyShape* pKey = &keyShape[1]; pKey < pKeyEnd; pKey++) {
                    f32 keyWeight = pKey->weight;
                    const ut::Color* pKeyVtx = pKey->pVtx++;

                    r += fastcast::U8_0ToF32(&pKeyVtx->r) * keyWeight;
                    g += fastcast::U8_0ToF32(&pKeyVtx->g) * keyWeight;
                    b += fastcast::U8_0ToF32(&pKeyVtx->b) * keyWeight;
                    a += fastcast::U8_0ToF32(&pKeyVtx->a) * keyWeight;
                }

                pVtxClrBuf->r = fastcast::F32ToU8_0(r);
                pVtxClrBuf->g = fastcast::F32ToU8_0(g);
                pVtxClrBuf->b = fastcast::F32ToU8_0(b);
                pVtxClrBuf->a = fastcast::F32ToU8_0(a);
            }

            DC::StoreRange(vtxClr.GetData(), numVtx * sizeof(ut::Color));
        }
    }
}

} // namespace g3d
} // namespace nw4r
