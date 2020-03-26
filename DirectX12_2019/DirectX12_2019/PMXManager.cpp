#include "PMXManager.h"

namespace{
	// ウェイト変換方式ごとのボーンIdxの個数
	constexpr int BDEF2bIndexCnt = 2;
	constexpr int BDEF4bIndexCnt = 4;
	constexpr int SDEFbIndexCnt = 2;
	// BDEF4のボーンウェイト値の個数
	constexpr int BDEF4bWeightCnt = 4;
	// SDEFのSDEF値の個数
	constexpr int SDEFValCnt = 3;
}

PMXManager::PMXManager()
{
}

PMXManager::~PMXManager()
{
}

bool PMXManager::LoadModel(std::string fileName)
{
	PMXHeader pmxHeader;
	FILE* fp = nullptr;
	fopen_s(&fp, fileName.c_str(), "rb");
	// PMXHeader読み込み
	if (fread_s(&pmxHeader, sizeof(pmxHeader.magic) + sizeof(pmxHeader.version),
		sizeof(pmxHeader.magic) + sizeof(pmxHeader.version), 1, fp) == 0) {
		return false;
	}
	if (fread_s(&pmxHeader.dataSize, sizeof(pmxHeader.dataSize), sizeof(pmxHeader.dataSize), 1, fp) == 0) {
		return false;
	}
	pmxHeader.sizeData.resize(pmxHeader.dataSize);
	if (fread_s(pmxHeader.sizeData.data(), pmxHeader.dataSize, pmxHeader.dataSize, 1, fp) == 0) {
		return false;
	}

	// PMX情報読み込み
	PMXInfo info;
	// モデル名のバッファ長読み込み
	if (fread_s(&info.byteSizeM, sizeof(info.byteSizeM), sizeof(info.byteSizeM), 1, fp) == 0) {
		return false;
	}
	
	// エンコードでサイズを変える
	if (pmxHeader.sizeData[0] == 0) {
		info.modelName.resize(info.byteSizeM / 2);
	}
	else {
		info.modelName.resize(info.byteSizeM);
	}

	// モデル名読み込み
	if (fread_s((void*)info.modelName.data(), info.byteSizeM, info.byteSizeM, 1, fp) == 0) {
		return false;
	}

	// モデル名(英語)のバッファ長読み込み
	if (fread_s(&info.byteSizeME, sizeof(info.byteSizeME), sizeof(info.byteSizeME), 1, fp) == 0) {
		return false;
	}

	if (info.byteSizeME > 0) {
		// エンコードで(ry
		if (pmxHeader.sizeData[0] == 0) {
			info.modelNameE.resize(info.byteSizeME / 2);
		}
		else {
			info.modelNameE.resize(info.byteSizeME);
		}

		// コメント読み込み
		if (fread_s((void*)info.modelNameE.data(), info.byteSizeME, info.byteSizeME, 1, fp) == 0) {
			return false;
		}
	}

	// コメントのバッファ長読み込み
	if (fread_s(&info.byteSizeC, sizeof(info.byteSizeC), sizeof(info.byteSizeC), 1, fp) == 0) {
		return false;
	}
	
	// エンコ(ry
	if (pmxHeader.sizeData[0] == 0) {
		info.comment.resize(info.byteSizeC / 2);
	}
	else {
		info.comment.resize(info.byteSizeC);
	}

	// コメントの読み込み
	if (fread_s((void*)info.comment.data(), info.byteSizeC, info.byteSizeC, 1, fp) == 0) {
		return false;
	}

	// コメント(英語)のバッファ長読み込み
	if (fread_s(&info.byteSizeCE, sizeof(info.byteSizeCE), sizeof(info.byteSizeCE), 1, fp) == 0) {
		return false;
	}

	if (info.byteSizeCE > 0) {
		// エン(ry
		if (pmxHeader.sizeData[0] == 0) {
			info.commentE.resize(info.byteSizeCE / 2);
		}
		else {
			info.commentE.resize(info.byteSizeCE);
		}
		// コメント(英語)の読み込み
		if (fread_s((void*)info.commentE.data(), info.byteSizeCE, info.byteSizeCE, 1, fp) == 0) {
			return false;
		}
	}

	int VertCnt = 0;
	if (fread_s(&VertCnt, sizeof(VertCnt), sizeof(VertCnt), 1, fp) == 0) {
		return false;
	}

	vertInfo.resize(VertCnt);
	auto sz = sizeof(vertInfo[0].position) + sizeof(vertInfo[0].normal) + sizeof(vertInfo[0].uv);
	for (int i = 0; i < VertCnt; i++) {
		// 最初の頂点情報読み込み
		if (fread_s(&vertInfo[i], sz, sz, 1, fp) == 0) {
			return false;
		}
		// 追加UVの読み込み
		if (pmxHeader.sizeData[1] != 0) {
			vertInfo[i].addUV.resize(pmxHeader.sizeData[1]);
			for (auto& uv : vertInfo[i].addUV) {
				if (fread_s(&uv, sizeof(uv), sizeof(uv), 1, fp) == 0) {
					return false;
				}
			}
		}

		// ウェイト変換方式読み込み
		if (fread_s(&vertInfo[i].weightType, sizeof(vertInfo[i].weightType), sizeof(vertInfo[i].weightType), 1, fp) == 0) {
			return false;
		}

		// ボーンインデックスサイズの取得
		auto bIdxSize = pmxHeader.sizeData[5];
		// 各要素の最大容量分確保
		vertInfo[i].boneRefIdx.resize((size_t)4 * bIdxSize);
		vertInfo[i].boneWeight.resize(4);
		vertInfo[i].sdefValue.resize(3);
		auto bwSize = sizeof(vertInfo[i].boneWeight[0]);
		auto sdefSize = sizeof(vertInfo[i].sdefValue[0]);
		switch (vertInfo[i].weightType)
		{
		case 0:
			// BDEF1の時のボーン参照インデックス読み込み
			if (fread_s(vertInfo[i].boneRefIdx.data(), bIdxSize, bIdxSize, 1, fp) == 0) {
				return false;
			}
			break;
		case 1:
			// BDEF2の時のボーン参照インデックス読み込み
			for (auto j = 0; j < BDEF2bIndexCnt; j++) {
				if (fread_s(&vertInfo[i].boneRefIdx[j], bIdxSize, bIdxSize, 1, fp) == 0) {
					return false;
				}
			}

			// ボーンウェイトの読み込み
			if (fread_s(&vertInfo[i].boneWeight[0], bwSize, bwSize, 1, fp) == 0) {
				return false;
			}
			break;
		case 2:
			// BDEF4の時のボーン参照インデックス読み込み
			for (auto j = 0; j < BDEF4bIndexCnt; j++) {
				if (fread_s(&vertInfo[i].boneRefIdx[j], bIdxSize, bIdxSize, 1, fp) == 0) {
					return false;
				}
			}

			// ボーンウェイトの読み込み
			for (auto j = 0; j < BDEF4bWeightCnt; j++) {
				if (fread_s(&vertInfo[i].boneWeight[j], bwSize, bwSize, 1, fp) == 0) {
					return false;
				}
			}
			break;
		case 3:
			// SDEFの時のボーン参照インデックス読み込み
			for (auto j = 0; j < SDEFbIndexCnt; j++) {
				if (fread_s(&vertInfo[i].boneRefIdx[j], bIdxSize, bIdxSize, 1, fp) == 0) {
					return false;
				}
			}
			// ボーンウェイトの読み込み
			if (fread_s(&vertInfo[i].boneWeight[0], bwSize, bwSize, 1, fp) == 0) {
				return false;
			}
			// SDEF値の読み込み
			for (auto j = 0; j < SDEFValCnt; j++) {
				if (fread_s(&vertInfo[i].sdefValue[j], sdefSize, sdefSize, 1, fp) == 0) {
					return false;
				}
			}
			break;
		}

		if (fread_s(&vertInfo[i].edgeDiameter, sizeof(vertInfo[i].edgeDiameter), sizeof(vertInfo[i].edgeDiameter), 1, fp) == 0) {
			return false;
		}
	}
	
	int surfaceCnt = 0;
	

	// 面数読み込み
	if (fread_s(&surfaceCnt, sizeof(surfaceCnt), sizeof(surfaceCnt), 1, fp) == 0) {
		return false;
	}

	// 面情報の読み込み
	SurfaceList.resize(surfaceCnt);
	auto vertIdxSize = pmxHeader.sizeData[2];
	for (auto& surf : SurfaceList) {
		if (fread_s(&surf, vertIdxSize, vertIdxSize, 1, fp) == 0) {
			return false;
		}
	}

	// テクスチャ数の読み込み
	int TextureCnt = 0;
	if (fread_s(&TextureCnt, sizeof(TextureCnt), sizeof(TextureCnt), 1, fp) == 0) {
		return false;
	}
	
	
	texPathInfo.resize(TextureCnt);
	for (auto j = 0; j < TextureCnt; j++) {
		// テクスチャパスのバッファ長読み込み
		if (fread_s(&texPathInfo[j].texturePathSize, sizeof(texPathInfo[j].texturePathSize),
			sizeof(texPathInfo[j].texturePathSize), 1, fp) == 0) {
			return false;
		}
		if (pmxHeader.sizeData[0] == 0) {
			texPathInfo[j].texturePath.resize(texPathInfo[j].texturePathSize / 2);
		}
		else {
			texPathInfo[j].texturePath.resize(texPathInfo[j].texturePathSize);
		}
		if (fread_s((void*)texPathInfo[j].texturePath.data(), texPathInfo[j].texturePathSize, texPathInfo[j].texturePathSize, 1, fp) == 0) {
			return false;
		}
	}

	// マテリアル情報の数
	int materialCnt = 0;
	if (fread_s(&materialCnt, sizeof(materialCnt), sizeof(materialCnt), 1, fp) == 0) {
		return false;
	}

	pmxMatInfo.resize(materialCnt);
	for (auto& matinfo : pmxMatInfo) {
		// 材質名のバッファ長読み込み
		if (fread_s(&matinfo.matNameSize, sizeof(matinfo.matNameSize), sizeof(matinfo.matNameSize), 1, fp) == 0) {
			return false;
		}
		if (pmxHeader.sizeData[0] == 0) {
			matinfo.matName.resize(matinfo.matNameSize / 2);
		}
		else {
			matinfo.matName.resize(matinfo.matNameSize);
		}

		// 材質名読み込み
		if (fread_s((void*)matinfo.matName.data(), matinfo.matNameSize, matinfo.matNameSize, 1, fp) == 0) {
			return false;
		}

		// 材質名(英)のバッファ長読み込み
		if (fread_s(&matinfo.matNameSizeE, sizeof(matinfo.matNameSizeE), sizeof(matinfo.matNameSizeE), 1, fp) == 0) {
			return false;
		}
		// 材質名(英)がある場合にのみ処理
		if (matinfo.matNameSizeE > 0) {
			// 材質名(英)の読み込み
			if (pmxHeader.sizeData[0] == 0) {
				matinfo.matNameE.resize(matinfo.matNameSizeE / 2);
			}
			else {
				matinfo.matNameE.resize(matinfo.matNameSizeE);
			}
			if (fread_s((void*)matinfo.matNameE.data(), matinfo.matNameSizeE, matinfo.matNameSizeE, 1, fp) == 0) {
				return false;
			}
		}

		// Diffuse,Specular,Specular係数,Ambient読み込み
		auto size = sizeof(matinfo.diffuse) + sizeof(matinfo.specular)
			+ sizeof(matinfo.specularVal) + sizeof(matinfo.ambient);
		if (fread_s(&matinfo.diffuse, size, size, 1, fp) == 0) {
			return false;
		}

		// 描画フラグ読み込み
		if (fread_s(&matinfo.drawFlag, sizeof(matinfo.drawFlag), sizeof(matinfo.drawFlag), 1, fp) == 0) {
			return false;
		}

		// エッジ色、サイズ読み込み
		size = sizeof(matinfo.edgeColor) + sizeof(matinfo.edgeSize);
		if (fread_s(&matinfo.edgeColor, size, size, 1, fp) == 0) {
			return false;
		}

		// 通常テクスチャなどの参照インデックス読み込み
		if (fread_s(&matinfo.normalTextureIdx, pmxHeader.sizeData[3], pmxHeader.sizeData[3], 1, fp) == 0) {
			return false;
		}

		// スフィアテクスチャなどの参照インデックス読み込み
		if (fread_s(&matinfo.spTextureIdx, pmxHeader.sizeData[3], pmxHeader.sizeData[3], 1, fp) == 0) {
			return false;
		}

		// スフィアモード読み込み
		if (fread_s(&matinfo.sphereMode, sizeof(matinfo.sphereMode), sizeof(matinfo.sphereMode), 1, fp) == 0) {
			return false;
		}

		// 共有トゥーンフラグ読み込み
		if (fread_s(&matinfo.shereToonFlg, sizeof(matinfo.shereToonFlg), sizeof(matinfo.shereToonFlg), 1, fp) == 0) {
			return false;
		}

		if (matinfo.shereToonFlg == 1) {
			// 共有トゥーンテクスチャ読み込み
			if (fread_s(&matinfo.shereToonTexRefIdx, sizeof(matinfo.shereToonTexRefIdx), sizeof(matinfo.shereToonTexRefIdx), 1, fp) == 0) {
				return false;
			}
		}
		else {
			// 個別トゥーン読み込み
			if (fread_s(&matinfo.toonTexRefIdx, pmxHeader.sizeData[3], pmxHeader.sizeData[3], 1, fp) == 0) {
				return false;
			}
		}

		// メモのバッファ長を読み込む
		if (fread_s(&matinfo.memoSize, sizeof(matinfo.memoSize), sizeof(matinfo.memoSize), 1, fp) == 0) {
			return false;
		}

		if (pmxHeader.sizeData[0] == 0) {
			matinfo.memo.resize(matinfo.memoSize / 2);
		}
		else {
			matinfo.memo.resize(matinfo.memoSize);
		}

		// メモ読み込み
		if (matinfo.memoSize > 0) {
			if (fread_s((void*)matinfo.memo.data(), matinfo.memoSize, matinfo.memoSize, 1, fp) == 0) {
				return false;
			}
		}

		// 材質に対応する面数読み込み
		if (fread_s(&matinfo.matSurfaceCnt, sizeof(matinfo.matSurfaceCnt), sizeof(matinfo.matSurfaceCnt), 1, fp) == 0) {
			return false;
		}
	}

	// ボーン数読み込み
	int boneNum = 0;
	if (fread_s(&boneNum, sizeof(boneNum), sizeof(boneNum), 1, fp) == 0) {
		return false;
	}
	
	boneInfos.resize(boneNum);
	// ボーン情報読み込み
	for (auto& binfo : boneInfos) {
		// ボーン名バッファ長読み込み
		if (fread_s(&binfo.boneNameSize, sizeof(binfo.boneNameSize), sizeof(binfo.boneNameSize), 1, fp) == 0) {
			return false;
		}
		if (pmxHeader.sizeData[0] == 0) {
			binfo.boneName.resize(binfo.boneNameSize / 2);
		}
		else {
			binfo.boneName.resize(binfo.boneNameSize);
		}
		// ボーン名読み込み
		if (fread_s((void*)binfo.boneName.data(), binfo.boneNameSize, binfo.boneNameSize, 1, fp) == 0) {
			return false;
		}

		// ボーン名(英)のバッファ長読み込み
		if (fread_s(&binfo.boneNameSizeE, sizeof(binfo.boneNameSizeE), sizeof(binfo.boneNameSizeE), 1, fp) == 0) {
			return false;
		}
		if (pmxHeader.sizeData[0] == 0) {
			binfo.boneNameE.resize(binfo.boneNameSizeE / 2);
		}
		else {
			binfo.boneNameE.resize(binfo.boneNameSizeE);
		}
		// ボーン名(英)の読み込み
		if (binfo.boneNameSizeE != 0) {
			if (fread_s((void*)binfo.boneNameE.data(), binfo.boneNameSizeE, binfo.boneNameSizeE, 1, fp) == 0) {
				return false;
			}
		}
		
		// 位置、親ボーンインデックス、変形階層読み込み
		sz = sizeof(binfo.position) + pmxHeader.sizeData[5] + sizeof(binfo.transformHierarchy);
		if (fread_s(&binfo.position, sz, sz, 1, fp) == 0) {
			return false;
		}

		// ボーンフラグ読み込み
		if (fread_s(&binfo.boneFlag, sizeof(binfo.boneFlag), sizeof(binfo.boneFlag), 1, fp) == 0) {
			return false;
		}

		// 接続先読み込み
		if (binfo.boneFlag & 0x0001) {
			if (fread_s(&binfo.connectBoneIdx, pmxHeader.sizeData[5], pmxHeader.sizeData[5], 1, fp) == 0) {
				return false;
			}
		}
		else {
			if (fread_s(&binfo.posOffset, sizeof(binfo.posOffset), sizeof(binfo.posOffset), 1, fp) == 0) {
				return false;
			}
		}

		// 回転付与情報読み込み
		if ((binfo.boneFlag & 0x0100) || (binfo.boneFlag & 0x0200)) {
			if (fread_s(&binfo.grantParentBoneIdx, pmxHeader.sizeData[5], pmxHeader.sizeData[5], 1, fp) == 0) {
				return false;
			}
			if (fread_s(&binfo.grantValue, sizeof(binfo.grantValue), sizeof(binfo.grantValue), 1, fp) == 0) {
				return false;
			}
		}

		// 軸固定情報読み込み
		if (binfo.boneFlag & 0x0400) {
			if (fread_s(&binfo.axisDirVec, sizeof(binfo.axisDirVec), sizeof(binfo.axisDirVec), 1, fp) == 0) {
				return false;
			}
		}

		// ローカル軸情報読み込み
		sz = sizeof(binfo.DirVecXAxis) + sizeof(binfo.DirVecZAxis);
		if (binfo.boneFlag & 0x0800) {
			if (fread_s(&binfo.DirVecXAxis, sz, sz, 1, fp) == 0) {
				return false;
			}
		}

		// 外部親変形情報読み込み
		if (binfo.boneFlag & 0x2000) {
			if (fread_s(&binfo.keyValue, sizeof(binfo.keyValue), sizeof(binfo.keyValue), 1, fp) == 0) {
				return false;
			}
		}

		// IK情報読み込み
		if (binfo.boneFlag & 0x0020) {
			if (fread_s(&binfo.IKTargetBoneIdx, pmxHeader.sizeData[5], pmxHeader.sizeData[5], 1, fp) == 0) {
				return false;
			}
			sz = sizeof(binfo.IKRoopCnt) + sizeof(binfo.IKLimAngle) + sizeof(binfo.IKLinkCnt);
			if (fread_s(&binfo.IKRoopCnt, sz, sz, 1, fp) == 0) {
				return false;
			}

			binfo.IKLinkInfos.resize(binfo.IKLinkCnt);
			for (auto& linkInfo : binfo.IKLinkInfos) {
				if (fread_s(&linkInfo.linkBoneIdx, pmxHeader.sizeData[5], pmxHeader.sizeData[5], 1, fp) == 0) {
					return false;
				}
				if (fread_s(&linkInfo.limitedAngle, sizeof(linkInfo.limitedAngle), sizeof(linkInfo.limitedAngle), 1, fp) == 0) {
					return false;
				}
				if (linkInfo.limitedAngle) {
					sz = sizeof(linkInfo.underLimitAngle) + sizeof(linkInfo.upperLimitAngle);
					if (fread_s(&linkInfo.underLimitAngle, sz, sz, 1, fp) == 0) {
						return false;
					}
				}
			}
		}
	}

	// モーフ情報読み込み
	int morphCnt = 0;
	if (fread_s(&morphCnt, sizeof(morphCnt), sizeof(morphCnt), 1, fp) == 0) {
		return false;
	}
	
	mInfos.resize(morphCnt);
	for (auto& minfo : mInfos) {
		// モーフ名の読み込み
		if (fread_s(&minfo.morphNameSize, sizeof(minfo.morphNameSize), sizeof(minfo.morphNameSize), 1, fp) == 0) {
			return false;
		}

		if (pmxHeader.sizeData[0] == 0) {
			
			minfo.morphName.resize(minfo.morphNameSize / 2);
		}
		else {
			minfo.morphName.resize(minfo.morphNameSize);
		}

		if (fread_s((void *)minfo.morphName.data(), minfo.morphNameSize, minfo.morphNameSize, 1, fp) == 0) {
			return false;
		}

		// モーフ(英)の読み込み
		if (fread_s(&minfo.morphNameSizeE, sizeof(minfo.morphNameSizeE), sizeof(minfo.morphNameSizeE), 1, fp) == 0) {
			return false;
		}

		if (pmxHeader.sizeData[0] == 0) {

			minfo.morphNameE.resize(minfo.morphNameSizeE / 2);
		}
		else {
			minfo.morphNameE.resize(minfo.morphNameSizeE);
		}

		if (minfo.morphNameSizeE != 0) {
			if (fread_s((void*)minfo.morphNameE.data(), minfo.morphNameSizeE, minfo.morphNameSizeE, 1, fp) == 0) {
				return false;
			}
		}

		// 操作パネル、モーフ種類情報の読み込み
		if (fread_s(&minfo.controlPanel, sizeof(minfo.controlPanel) + sizeof(minfo.morphType), 
			sizeof(minfo.controlPanel) + sizeof(minfo.morphType), 1, fp) == 0) {
			return false;
		}

		// モーフのオフセット数読み込み
		if (fread_s(&minfo.morphOffsetCnt, sizeof(minfo.morphOffsetCnt), sizeof(minfo.morphOffsetCnt), 1, fp) == 0) {
			return false;
		}

		switch (minfo.morphType)
		{
		case 0:
			minfo.groupMorphs.resize(minfo.morphOffsetCnt);
			for (auto& gMorph : minfo.groupMorphs) {
				if (fread_s(&gMorph.morphIdx, pmxHeader.sizeData[6], pmxHeader.sizeData[6], 1, fp) == 0) {
					return false;
				}
				if (fread_s(&gMorph.MorphVal, sizeof(gMorph.MorphVal), sizeof(gMorph.MorphVal), 1, fp) == 0) {
					return false;
				}
			}
			break;
		case 1:
			minfo.vertexMorphs.resize(minfo.morphOffsetCnt);
			for (auto& vMorph : minfo.vertexMorphs) {
				if (fread_s(&vMorph.VertIndex, pmxHeader.sizeData[2], pmxHeader.sizeData[2], 1, fp) == 0) {
					return false;
				}
				if (fread_s(&vMorph.posOffset, sizeof(vMorph.posOffset), sizeof(vMorph.posOffset), 1, fp) == 0) {
					return false;
				}
			}
			break;
		case 2:
			minfo.boneMorphs.resize(minfo.morphOffsetCnt);
			for (auto& bMorph : minfo.boneMorphs) {
				if (fread_s(&bMorph.boneIdx, pmxHeader.sizeData[5], pmxHeader.sizeData[5], 1, fp) == 0) {
					return false;
				}
				sz = sizeof(bMorph.movementVal) + sizeof(bMorph.rotationVal);
				if (fread_s(&bMorph.movementVal, sz, sz, 1, fp) == 0) {
					return false;
				}
			}
			break;
		case 3:
			minfo.uvMorphs.resize(minfo.morphOffsetCnt);
			for (auto& uMorph : minfo.uvMorphs) {
				if (fread_s(&uMorph.VertIndex, pmxHeader.sizeData[2], pmxHeader.sizeData[2], 1, fp) == 0) {
					return false;
				}
				if (fread_s(&uMorph.uvOffset, sizeof(uMorph.uvOffset), sizeof(uMorph.uvOffset), 1, fp) == 0){
					return false;
				}
			}
			break;
		case 4:
		case 5:
		case 6:
		case 7:
			minfo.addUvMorphs.resize(minfo.morphOffsetCnt);
			for (auto& adduMorph : minfo.addUvMorphs) {
				if (fread_s(&adduMorph.VertIndex, pmxHeader.sizeData[2], pmxHeader.sizeData[2], 1, fp) == 0) {
					return false;
				}
				if (fread_s(&adduMorph.uvOffset, sizeof(adduMorph.uvOffset), sizeof(adduMorph.uvOffset), 1, fp) == 0) {
					return false;
				}
			}
			break;
		case 8:
			minfo.materialMorphs.resize(minfo.morphOffsetCnt);
			for (auto& matMorph : minfo.materialMorphs) {
				if (fread_s(&matMorph.materialIdx, pmxHeader.sizeData[4], pmxHeader.sizeData[4], 1, fp) == 0) {
					return false;
				}
				if (fread_s(&matMorph.offsetCalculation, sizeof(matMorph.offsetCalculation), sizeof(matMorph.offsetCalculation), 1, fp) == 0) {
					return false;
				}
				sz = sizeof(matMorph.diffuse) + sizeof(matMorph.specular) + sizeof(matMorph.specularVal) + sizeof(matMorph.ambient) + sizeof(matMorph.edgeColor)
					+ sizeof(matMorph.edgeSize) + sizeof(matMorph.textureVal) + sizeof(matMorph.sphereTextureVal) + sizeof(matMorph.toonTextureVal);
				if (fread_s(&matMorph.diffuse, sz, sz, 1, fp) == 0) {
					return false;
				}
			}
			break;
		default:
			break;
		}
	}
	

	auto a = 0;

	return true;
}

std::vector<VertexInfo> PMXManager::GetPMXVertices()
{
	return vertInfo;
}

std::vector<unsigned int> PMXManager::GetPMXIndices()
{
	return SurfaceList;
}

std::vector<PMXMaterialInfo> PMXManager::GetPMXMaterials()
{
	return pmxMatInfo;
}

std::vector<TexturePathInfo> PMXManager::GetPMXTexturePath()
{
	return texPathInfo;
}

std::vector<BoneInfo> PMXManager::GetPMXBoneInfo()
{
	return boneInfos;
}

std::vector<MorphInfo> PMXManager::GetPMXMorphInfo()
{
	return mInfos;
}

std::string PMXManager::GetToonFileName(int idx)
{
	return toonFileNames[idx];
}
