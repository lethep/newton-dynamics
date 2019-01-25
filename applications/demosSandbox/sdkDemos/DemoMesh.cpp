/* Copyright (c) <2003-2016> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/


#include "toolbox_stdafx.h"
#include "DemoMesh.h"
#include "DemoEntity.h"
#include "TargaToOpenGl.h"
#include "DemoEntityManager.h"

dInitRtti(DemoMeshInterface);
dInitRtti(DemoMesh);
dInitRtti(DemoBezierCurve);


#define USING_DISPLAY_LIST



#if defined(__APPLE__)
// NOTE: displaylists are horribly slow on OSX
// they cut the framerate in half
#	if defined(USING_DISPLAY_LIST)
#		undef USING_DISPLAY_LIST
#	endif
#endif

DemoMeshInterface::DemoMeshInterface()
	:dClassInfo()
	,m_name()
	,m_isVisible(true)
{
}

DemoMeshInterface::~DemoMeshInterface()
{
}

const dString& DemoMeshInterface::GetName () const
{
	//	strcpy (nameOut, m_name);
	return m_name;
}


bool DemoMeshInterface::GetVisible () const
{
	return m_isVisible;
}

void DemoMeshInterface::SetVisible (bool visibilityFlag)
{
	m_isVisible = visibilityFlag;
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
DemoSubMesh::DemoSubMesh ()
	:m_indexCount(0)
	,m_indexes(NULL)
	,m_textureHandle(0)
	,m_shiness(80.0f)
	,m_ambient (0.8f, 0.8f, 0.8f, 1.0f)
	,m_diffuse (0.8f, 0.8f, 0.8f, 1.0f)
	,m_specular (1.0f, 1.0f, 1.0f, 1.0f)
	,m_opacity(1.0f)
{
}

DemoSubMesh::~DemoSubMesh ()
{
	if (m_textureHandle) {
		ReleaseTexture(m_textureHandle);
	}

	if (m_indexes) {
		delete[] m_indexes;
	}
}

void DemoSubMesh::SetOpacity(dFloat opacity)
{
	m_opacity = opacity;
	m_ambient.m_w = opacity;
	m_diffuse.m_w = opacity;
	m_specular.m_w = opacity;
}

void DemoSubMesh::Render() const
{
	glMaterialParam(GL_FRONT, GL_SPECULAR, &m_specular.m_x);
	glMaterialParam(GL_FRONT, GL_AMBIENT, &m_ambient.m_x);
	glMaterialParam(GL_FRONT, GL_DIFFUSE, &m_diffuse.m_x);
	glMaterialf(GL_FRONT, GL_SHININESS, GLfloat(m_shiness));
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	if (m_textureHandle) {
		glEnable(GL_TEXTURE_2D);		
		glBindTexture(GL_TEXTURE_2D, GLuint (m_textureHandle));
	} else {
		glDisable(GL_TEXTURE_2D);
	}

	glDrawElements (GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, m_indexes);
}

void DemoSubMesh::OptimizeForRender(const DemoMesh* const mesh) const
{
	glMaterialParam(GL_FRONT, GL_SPECULAR, &m_specular.m_x);
	glMaterialParam(GL_FRONT, GL_AMBIENT, &m_ambient.m_x);
	glMaterialParam(GL_FRONT, GL_DIFFUSE, &m_diffuse.m_x);
	glMaterialf(GL_FRONT, GL_SHININESS, GLfloat(m_shiness));
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	if (m_textureHandle) {
		glEnable(GL_TEXTURE_2D);		
		glBindTexture(GL_TEXTURE_2D, GLuint (m_textureHandle));
	} else {
		glDisable(GL_TEXTURE_2D);
	}

	glBegin(GL_TRIANGLES);
	const dFloat* const uv = mesh->m_uv;
	const dFloat* const normal = mesh->m_normal;
	const dFloat* const vertex = mesh->m_vertex;
	for (int i = 0; i < m_indexCount; i ++) {
		int index = m_indexes[i];
		glTexCoord2f(GLfloat(uv[index * 2 + 0]), GLfloat(uv[index * 2 + 1])); 
		glNormal3f (GLfloat(normal[index * 3 + 0]), GLfloat(normal[index * 3 + 1]), GLfloat(normal[index * 3 + 2])); 
		glVertex3f(GLfloat(vertex[index * 3 + 0]), GLfloat(vertex[index * 3 + 1]), GLfloat(vertex[index * 3 + 2]));
	}

	glEnd();
}

void DemoSubMesh::AllocIndexData (int indexCount)
{
	m_indexCount = indexCount;
	if (m_indexes) {
		delete[] m_indexes;
	}
	m_indexes = new unsigned [m_indexCount]; 
}

DemoMesh::DemoMesh(const char* const name)
	:DemoMeshInterface()
	,dList<DemoSubMesh>()
	,m_vertexCount(0)
	,m_uv (NULL)
	,m_vertex(NULL)
	,m_normal(NULL)
	,m_optimizedOpaqueDiplayList(0)	
	,m_optimizedTransparentDiplayList(0)
{
}

DemoMesh::DemoMesh(const dScene* const scene, dScene::dTreeNode* const meshNode)
	:DemoMeshInterface()
	,dList<DemoSubMesh>()
	,m_uv(NULL)
	,m_vertex(NULL)
	,m_normal(NULL)
	,m_optimizedOpaqueDiplayList(0)
	,m_optimizedTransparentDiplayList(0)
{
	dMeshNodeInfo* const meshInfo = (dMeshNodeInfo*)scene->GetInfoFromNode(meshNode);
	m_name = meshInfo->GetName();
	
	NewtonMesh* const mesh = meshInfo->GetMesh();

	// recalculate mesh normals
//	NewtonMeshCalculateVertexNormals(mesh, 45.0f * dDegreeToRad);

	// extract vertex data  from the newton mesh		
	AllocVertexData(NewtonMeshGetPointCount (mesh));
	NewtonMeshGetVertexChannel (mesh, 3 * sizeof (dFloat), (dFloat*) m_vertex);
	NewtonMeshGetNormalChannel (mesh, 3 * sizeof (dFloat), (dFloat*) m_normal);
	NewtonMeshGetUV0Channel (mesh, 2 * sizeof (dFloat), (dFloat*) m_uv);

	// bake the matrix into the vertex array
//	dMatrix matrix (meshInfo->GetPivotMatrix());
//	matrix.TransformTriplex(m_vertex, 3 * sizeof (dFloat), m_vertex, 3 * sizeof (dFloat), m_vertexCount);
//	matrix.m_posit = dVector (0.0f, 0.0f, 0.0f, 1.0f);
//	matrix = (matrix.Inverse4x4()).Transpose();
//	matrix.TransformTriplex(m_normal, 3 * sizeof (dFloat), m_normal, 3 * sizeof (dFloat), m_vertexCount);

//	bool hasModifiers = false;
	dTree<dScene::dTreeNode*, dCRCTYPE> materialMap;
	for (void* ptr = scene->GetFirstChildLink(meshNode); ptr; ptr = scene->GetNextChildLink (meshNode, ptr)) {
		dScene::dTreeNode* const node = scene->GetNodeFromLink(ptr);
		dNodeInfo* const info = scene->GetInfoFromNode(node);
		if (info->GetTypeId() == dMaterialNodeInfo::GetRttiType()) {
			dMaterialNodeInfo* const material = (dMaterialNodeInfo*)info;
			dCRCTYPE id = material->GetId();
			materialMap.Insert(node, id);
//		} else if (info->IsType(dGeometryNodeModifierInfo::GetRttiType())) {
//			hasModifiers = true;
		}
	}

	// extract the materials index array for mesh
	void* const meshCookie = NewtonMeshBeginHandle (mesh); 
	for (int handle = NewtonMeshFirstMaterial (mesh, meshCookie); handle != -1; handle = NewtonMeshNextMaterial (mesh, meshCookie, handle)) {
		int materialIndex = NewtonMeshMaterialGetMaterial (mesh, meshCookie, handle); 
		int indexCount = NewtonMeshMaterialGetIndexCount (mesh, meshCookie, handle); 
		DemoSubMesh* const segment = AddSubMesh();

		dTree<dScene::dTreeNode*, dCRCTYPE>::dTreeNode* matNodeCache = materialMap.Find(materialIndex);
		if (matNodeCache) {
			dScene::dTreeNode* const matNode = matNodeCache->GetInfo();
			dMaterialNodeInfo* const material = (dMaterialNodeInfo*) scene->GetInfoFromNode(matNode);

			if (material->GetDiffuseTextId() != -1) {
				dScene::dTreeNode* const node = scene->FindTextureByTextId(matNode, material->GetDiffuseTextId());
				dAssert (node);
				if (node) {
					dTextureNodeInfo* const texture = (dTextureNodeInfo*)scene->GetInfoFromNode(node);
					segment->m_textureHandle = LoadTexture(texture->GetPathName());
					segment->m_textureName = texture->GetPathName();
				}
			}
			segment->m_shiness = material->GetShininess();
			segment->m_ambient = material->GetAmbientColor();
			segment->m_diffuse = material->GetDiffuseColor();
			segment->m_specular = material->GetSpecularColor();
			segment->SetOpacity(material->GetOpacity());
		}

		segment->AllocIndexData (indexCount);
		// for 16 bit indices meshes
		//NewtonMeshMaterialGetIndexStreamShort (mesh, meshCookie, handle, (short int*)segment->m_indexes); 

		// for 32 bit indices mesh
		NewtonMeshMaterialGetIndexStream (mesh, meshCookie, handle, (int*)segment->m_indexes); 
	}
	NewtonMeshEndHandle (mesh, meshCookie); 

//	if (!hasModifiers) {
		// see if this mesh can be optimized
		OptimizeForRender ();
//	}
}

DemoMesh::DemoMesh(NewtonMesh* const mesh)
	:DemoMeshInterface()
	,m_uv(NULL)
	,m_vertex(NULL)
	,m_normal(NULL)
	,m_optimizedOpaqueDiplayList(0)		
	,m_optimizedTransparentDiplayList(0)
{
	// extract vertex data  from the newton mesh		
	AllocVertexData(NewtonMeshGetPointCount (mesh));

	// a valid newton mesh always has a vertex channel
	NewtonMeshGetVertexChannel(mesh, 3 * sizeof (dFloat), (dFloat*)m_vertex);
	if (NewtonMeshHasNormalChannel(mesh)) {
		NewtonMeshGetNormalChannel(mesh, 3 * sizeof (dFloat), (dFloat*)m_normal);
	}
	if (NewtonMeshHasUV0Channel(mesh)) {
		NewtonMeshGetUV0Channel(mesh, 2 * sizeof (dFloat), (dFloat*)m_uv);
	}

	// extract the materials index array for mesh
	void* const meshCookie = NewtonMeshBeginHandle (mesh); 
	for (int handle = NewtonMeshFirstMaterial (mesh, meshCookie); handle != -1; handle = NewtonMeshNextMaterial (mesh, meshCookie, handle)) {
		int textureId = NewtonMeshMaterialGetMaterial (mesh, meshCookie, handle); 
		int indexCount = NewtonMeshMaterialGetIndexCount (mesh, meshCookie, handle); 
		DemoSubMesh* const segment = AddSubMesh();

		segment->m_shiness = 1.0f;
		segment->m_ambient = dVector (0.8f, 0.8f, 0.8f, 1.0f);
		segment->m_diffuse = dVector (0.8f, 0.8f, 0.8f, 1.0f);
		segment->m_specular = dVector (0.0f, 0.0f, 0.0f, 1.0f);
		segment->m_textureHandle = textureId;

		segment->AllocIndexData (indexCount);
		// for 16 bit indices meshes
		//NewtonMeshMaterialGetIndexStreamShort (mesh, meshCookie, handle, (short int*)segment->m_indexes); 

		// for 32 bit indices mesh
		NewtonMeshMaterialGetIndexStream (mesh, meshCookie, handle, (int*)segment->m_indexes); 
	}
	NewtonMeshEndHandle (mesh, meshCookie); 

	// see if this mesh can be optimized
	OptimizeForRender ();
}

DemoMesh::DemoMesh(const DemoMesh& mesh)
	:DemoMeshInterface()
	,dList<DemoSubMesh>()
	,m_uv(NULL)
	,m_vertex(NULL)
	,m_normal(NULL)
	,m_optimizedOpaqueDiplayList(0)		
	,m_optimizedTransparentDiplayList(0)
{
	AllocVertexData(mesh.m_vertexCount);
	memcpy (m_vertex, mesh.m_vertex, 3 * m_vertexCount * sizeof (dFloat));
	memcpy (m_normal, mesh.m_normal, 3 * m_vertexCount * sizeof (dFloat));
	memcpy (m_uv, mesh.m_uv, 2 * m_vertexCount * sizeof (dFloat));

	for (dListNode* nodes = mesh.GetFirst(); nodes; nodes = nodes->GetNext()) {
		DemoSubMesh* const segment = AddSubMesh();
		DemoSubMesh& srcSegment = nodes->GetInfo();

		segment->AllocIndexData (srcSegment.m_indexCount);
		memcpy (segment->m_indexes, srcSegment.m_indexes, srcSegment.m_indexCount * sizeof (unsigned));

		segment->m_shiness = srcSegment.m_shiness;
		segment->m_ambient = srcSegment.m_ambient;
		segment->m_diffuse = srcSegment.m_diffuse;
		segment->m_specular = srcSegment.m_specular;
		segment->m_textureHandle = srcSegment.m_textureHandle;
		segment->m_textureName = srcSegment.m_textureName;
		if (segment->m_textureHandle) {
			AddTextureRef (srcSegment.m_textureHandle);
		}
	}

	// see if this mesh can be optimized
	OptimizeForRender ();
}

DemoMesh::DemoMesh(const char* const name, const NewtonCollision* const collision, const char* const texture0, const char* const texture1, const char* const texture2, dFloat opacity, const dMatrix& uvMatrix)
	:DemoMeshInterface()
	,dList<DemoSubMesh>()
	,m_uv(NULL)
	,m_vertex(NULL)
	,m_normal(NULL)
	,m_optimizedOpaqueDiplayList(0)		
	,m_optimizedTransparentDiplayList(0)
{
	// create a helper mesh from the collision collision
	NewtonMesh* const mesh = NewtonMeshCreateFromCollision(collision);

	// apply the vertex normals
//	NewtonMeshCalculateVertexNormals(mesh, 30.0f * dDegreeToRad);

	dMatrix aligmentUV(uvMatrix);
//	NewtonCollisionGetMatrix(collision, &aligmentUV[0][0]);
	aligmentUV = aligmentUV.Inverse();

	// apply uv projections
	NewtonCollisionInfoRecord info;
	NewtonCollisionGetInfo (collision, &info);
	switch (info.m_collisionType) 
	{
		case SERIALIZE_ID_SPHERE:
		{
			NewtonMeshApplySphericalMapping(mesh, LoadTexture (texture0), &aligmentUV[0][0]);
			break;
		}

		case SERIALIZE_ID_CONE:
		case SERIALIZE_ID_CAPSULE:
		case SERIALIZE_ID_CYLINDER:
		case SERIALIZE_ID_CHAMFERCYLINDER:
		{
			//NewtonMeshApplySphericalMapping(mesh, LoadTexture(texture0));
			NewtonMeshApplyCylindricalMapping(mesh, LoadTexture(texture0), LoadTexture(texture1), &aligmentUV[0][0]);
			break;
		}

		default:
		{
			int tex0 = LoadTexture(texture0);
			int tex1 = LoadTexture(texture1);
			int tex2 = LoadTexture(texture2);
			NewtonMeshApplyBoxMapping(mesh, tex0, tex1, tex2, &aligmentUV[0][0]);
			break;
		}
	}

	// extract vertex data  from the newton mesh		
	int vertexCount = NewtonMeshGetPointCount (mesh); 
	AllocVertexData(vertexCount);
	NewtonMeshGetVertexChannel(mesh, 3 * sizeof (dFloat), (dFloat*)m_vertex);
	NewtonMeshGetNormalChannel(mesh, 3 * sizeof (dFloat), (dFloat*)m_normal);
	NewtonMeshGetUV0Channel(mesh, 2 * sizeof (dFloat), (dFloat*)m_uv);

	// extract the materials index array for mesh
	void* const geometryHandle = NewtonMeshBeginHandle (mesh); 
	for (int handle = NewtonMeshFirstMaterial (mesh, geometryHandle); handle != -1; handle = NewtonMeshNextMaterial (mesh, geometryHandle, handle)) {
		int material = NewtonMeshMaterialGetMaterial (mesh, geometryHandle, handle); 
		int indexCount = NewtonMeshMaterialGetIndexCount (mesh, geometryHandle, handle); 

		DemoSubMesh* const segment = AddSubMesh();

		segment->m_textureHandle = (GLuint)material;
		segment->SetOpacity(opacity);

		segment->AllocIndexData (indexCount);
		NewtonMeshMaterialGetIndexStream (mesh, geometryHandle, handle, (int*)segment->m_indexes); 
	}
	NewtonMeshEndHandle (mesh, geometryHandle); 

	// destroy helper mesh
	NewtonMeshDestroy(mesh);

	// optimize this mesh for hardware buffers if possible
	OptimizeForRender ();
}

DemoMesh::DemoMesh(const char* const name, dFloat* const elevation, int size, dFloat cellSize, dFloat texelsDensity, int tileSize)
	:DemoMeshInterface()
	,dList<DemoSubMesh>()
	,m_uv(NULL)
	,m_vertex(NULL)
	,m_normal(NULL)
	,m_optimizedOpaqueDiplayList(0)		
	,m_optimizedTransparentDiplayList(0)
{
	dFloat* elevationMap[4096];
	dVector* normalMap[4096];
	dFloat* const normalsPtr = new dFloat [size * size * 4];
//	dVector* const normals = new dVector [size * size];
	dVector* const normals = (dVector*)normalsPtr;

	for (int i = 0; i < size; i ++) {
		elevationMap[i] = &elevation[i * size];
		normalMap[i] = &normals[i * size];
	}

	memset (normals, 0, (size * size) * sizeof (dVector));
	for (int z = 0; z < size - 1; z ++) {
		for (int x = 0; x < size - 1; x ++) {
			dVector p0 ((x + 0) * cellSize, elevationMap[z + 0][x + 0], (z + 0) * cellSize);
			dVector p1 ((x + 1) * cellSize, elevationMap[z + 0][x + 1], (z + 0) * cellSize);
			dVector p2 ((x + 1) * cellSize, elevationMap[z + 1][x + 1], (z + 1) * cellSize);
			dVector p3 ((x + 0) * cellSize, elevationMap[z + 1][x + 0], (z + 1) * cellSize);

			dVector e10 (p1 - p0);
			dVector e20 (p2 - p0);
			dVector n0 (e20.CrossProduct(e10));
			n0 = n0.Scale ( 1.0f / dSqrt (n0.DotProduct3(n0)));
			normalMap [z + 0][x + 0] += n0;
			normalMap [z + 0][x + 1] += n0;
			normalMap [z + 1][x + 1] += n0;

			dVector e30 (p3 - p0);
			dVector n1 (e30.CrossProduct(e20));
			n1 = n1.Scale ( 1.0f / dSqrt (n1.DotProduct3(n1)));
			normalMap [z + 0][x + 0] += n1;
			normalMap [z + 1][x + 0] += n1;
			normalMap [z + 1][x + 1] += n1;
		}
	}

	for (int i = 0; i < size * size; i ++) {
		normals[i] = normals[i].Scale (1.0f / dSqrt (normals[i].DotProduct3(normals[i])));
	}
	
	AllocVertexData (size * size);

	dFloat* const vertex = m_vertex;
	dFloat* const normal = m_normal;
	dFloat* const uv = m_uv;

	int index0 = 0;
	for (int z = 0; z < size; z ++) {
		for (int x = 0; x < size; x ++) {
			vertex[index0 * 3 + 0] = x * cellSize;
			vertex[index0 * 3 + 1] = elevationMap[z][x];
			vertex[index0 * 3 + 2] = z * cellSize;

			normal[index0 * 3 + 0] = normalMap[z][x].m_x;
			normal[index0 * 3 + 1] = normalMap[z][x].m_y;
			normal[index0 * 3 + 2] = normalMap[z][x].m_z;

			uv[index0 * 2 + 0] = x * texelsDensity;
			uv[index0 * 2 + 1] = z * texelsDensity;
			index0 ++;
		}
	}

	int segmentsCount = (size - 1) / tileSize;
	for (int z0 = 0; z0 < segmentsCount; z0 ++) {
		int z = z0 * tileSize;
		for (int x0 = 0; x0 < segmentsCount; x0 ++ ) {
			int x = x0 * tileSize;

			DemoSubMesh* const tile = AddSubMesh();
			tile->AllocIndexData (tileSize * tileSize * 6);
			unsigned* const indexes = tile->m_indexes;

			//strcpy (tile->m_textureName, "grassanddirt.tga");
			tile->m_textureName = "grassanddirt.tga";
			tile->m_textureHandle = LoadTexture(tile->m_textureName.GetStr());

			int index1 = 0;
			int x1 = x + tileSize;
			int z1 = z + tileSize;
			for (int z2 = z; z2 < z1; z2 ++) {
				for (int x2 = x; x2 < x1; x2 ++) {
					int i0 = x2 + 0 + (z2 + 0) * size;
					int i1 = x2 + 1 + (z2 + 0) * size;
					int i2 = x2 + 1 + (z2 + 1) * size;
					int i3 = x2 + 0 + (z2 + 1) * size;

					indexes[index1 + 0] = i0;
					indexes[index1 + 1] = i2;
					indexes[index1 + 2] = i1;

					indexes[index1 + 3] = i0;
					indexes[index1 + 4] = i3;
					indexes[index1 + 5] = i2;
					index1 += 6;
				}
			}
		}
	}
	delete[] normalsPtr; 
	OptimizeForRender();
}

DemoMesh::~DemoMesh()
{
	if (m_vertex) {
		delete[] m_vertex;
		delete[] m_normal;
		delete[] m_uv;
		ResetOptimization();
	}
}

NewtonMesh* DemoMesh::CreateNewtonMesh(NewtonWorld* const world, const dMatrix& meshMatrix)
{
	NewtonMesh* const mesh = NewtonMeshCreate(world);

	NewtonMeshBeginBuild (mesh);
	dMatrix rotation ((meshMatrix.Inverse4x4()).Transpose4X4());

	for (dListNode* node = GetFirst(); node; node = node->GetNext()) {
		DemoSubMesh& segment = node->GetInfo();
		for (int i = 0; i < segment.m_indexCount; i += 3) {
			NewtonMeshBeginFace(mesh);
			for (int j = 0; j < 3; j ++) {
				int index = segment.m_indexes[i + j];
				dVector p (meshMatrix.TransformVector(dVector (m_vertex[index * 3 + 0], m_vertex[index * 3 + 1], m_vertex[index * 3 + 2], 0.0f)));
				dVector n (rotation.RotateVector(dVector (m_normal[index * 3 + 0], m_normal[index * 3 + 1], m_normal[index * 3 + 2], 0.0f)));
				dAssert ((n.DotProduct3(n)) > 0.0f);
				n = n.Scale (1.0f / dSqrt (n.DotProduct3(n)));
				
				NewtonMeshAddPoint(mesh, p.m_x, p.m_y, p.m_z);
				NewtonMeshAddNormal(mesh, n.m_x, n.m_y, n.m_z);
				NewtonMeshAddMaterial(mesh, segment.m_textureHandle);
				NewtonMeshAddUV0(mesh, m_uv[index * 2 + 0], m_uv[index * 2 + 1]);
			}
			NewtonMeshEndFace(mesh);
//			NewtonMeshAddFace(mesh, 3, &point[0][0], sizeof (point) / 3, segment.m_textureHandle);
		}
	}

	NewtonMeshEndBuild(mesh);
	return mesh;
}


const dString& DemoMesh::GetTextureName (const DemoSubMesh* const subMesh) const
{
//	strcpy (nameOut, subMesh->m_textureName);
	return subMesh->m_textureName;
}

void DemoMesh::SpliteSegment(dListNode* const node, int maxIndexCount)
{
	const DemoSubMesh& segment = node->GetInfo(); 
	if (segment.m_indexCount > maxIndexCount) {
		dVector minBox (1.0e10f, 1.0e10f, 1.0e10f, 0.0f);
		dVector maxBox (-1.0e10f, -1.0e10f, -1.0e10f, 0.0f);
		for (int i = 0; i < segment.m_indexCount; i ++) {
			int index = segment.m_indexes[i];
			for (int j = 0; j < 3; j ++) {
				minBox[j] = (m_vertex[index * 3 + j] < minBox[j]) ? m_vertex[index * 3 + j] : minBox[j];
				maxBox[j] = (m_vertex[index * 3 + j] > maxBox[j]) ? m_vertex[index * 3 + j] : maxBox[j];
			}
		}

		int index = 0;
		dFloat maxExtend = -1.0e10f;
		for (int j = 0; j < 3; j ++) {
			dFloat ext = maxBox[j] - minBox[j];
			if (ext > maxExtend ) {
				index = j;
				maxExtend = ext;
			}
		}

		int leftCount = 0;
		int rightCount = 0;
		dFloat spliteDist = (maxBox[index ] + minBox[index]) * 0.5f;
		for (int i = 0; i < segment.m_indexCount; i += 3) {
			bool isleft = true;
			for (int j = 0; j < 3; j ++) {
				int vertexIndex = segment.m_indexes[i + j];
				isleft &= (m_vertex[vertexIndex * 3 + index] < spliteDist);
			}
			if (isleft) {
				leftCount += 3;
			} else {
				rightCount += 3;
			}
		}
		dAssert (leftCount);
		dAssert (rightCount);

		dListNode* const leftNode = Append();
		dListNode* const rightNode = Append();
		DemoSubMesh* const leftSubMesh = &leftNode->GetInfo();
		DemoSubMesh* const rightSubMesh = &rightNode->GetInfo();
		leftSubMesh->AllocIndexData (leftCount);
		rightSubMesh->AllocIndexData (rightCount);

		leftSubMesh->m_textureHandle = AddTextureRef (segment.m_textureHandle);
		rightSubMesh->m_textureHandle = AddTextureRef (segment.m_textureHandle);
		leftSubMesh->m_textureName = segment.m_textureName;
		rightSubMesh->m_textureName = segment.m_textureName;

		leftCount = 0;
		rightCount = 0;
		for (int i = 0; i < segment.m_indexCount; i += 3) {
			bool isleft = true;
			for (int j = 0; j < 3; j ++) {
				int vertexIndex = segment.m_indexes[i + j];
				isleft &= (m_vertex[vertexIndex * 3 + index] < spliteDist);
			}
			if (isleft) {
				leftSubMesh->m_indexes[leftCount + 0] = segment.m_indexes[i + 0];
				leftSubMesh->m_indexes[leftCount + 1] = segment.m_indexes[i + 1];
				leftSubMesh->m_indexes[leftCount + 2] = segment.m_indexes[i + 2];
				leftCount += 3;
			} else {
				rightSubMesh->m_indexes[rightCount + 0] = segment.m_indexes[i + 0];
				rightSubMesh->m_indexes[rightCount + 1] = segment.m_indexes[i + 1];
				rightSubMesh->m_indexes[rightCount + 2] = segment.m_indexes[i + 2];
				rightCount += 3;
			}
		}

		SpliteSegment(leftNode, maxIndexCount);
		SpliteSegment(rightNode, maxIndexCount);
		Remove(node);
	}
}

void  DemoMesh::OptimizeForRender()
{
	// first make sure the previous optimization is removed
	ResetOptimization();

	dListNode* nextNode;
	for (dListNode* node = GetFirst(); node; node = nextNode) {
		DemoSubMesh& segment = node->GetInfo();
		nextNode = node->GetNext();
		if (segment.m_indexCount > 128 * 128 * 6) {
			SpliteSegment(node, 128 * 128 * 6);
		}
	}

#ifdef USING_DISPLAY_LIST
	bool isOpaque = false;
	bool hasTranparency = false;

	for (dListNode* node = GetFirst(); node; node = node->GetNext()) {
		DemoSubMesh& segment = node->GetInfo();
		isOpaque |= segment.m_opacity > 0.999f;
		hasTranparency |= segment.m_opacity <= 0.999f;
	}

	if (isOpaque) {
		m_optimizedOpaqueDiplayList = glGenLists(1);

		glNewList(m_optimizedOpaqueDiplayList, GL_COMPILE);

		//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

		for (dListNode* node = GetFirst(); node; node = node->GetNext()) {
			DemoSubMesh& segment = node->GetInfo();
			if (segment.m_opacity > 0.999f) {
				segment.OptimizeForRender(this);
			}
		}
		glEndList();
	}

	if (hasTranparency) {
        m_optimizedTransparentDiplayList = glGenLists(1);

        glNewList(m_optimizedTransparentDiplayList, GL_COMPILE);
        //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for (dListNode* node = GetFirst(); node; node = node->GetNext()) {
            DemoSubMesh& segment = node->GetInfo();
            if (segment.m_opacity <= 0.999f) {
                segment.OptimizeForRender(this);
            }
        }
		glDisable(GL_BLEND);
		glLoadIdentity();
        glEndList();
	}
#endif
}

void  DemoMesh::ResetOptimization()
{
	if (m_optimizedOpaqueDiplayList) {
		glDeleteLists(m_optimizedOpaqueDiplayList, 1);
		m_optimizedOpaqueDiplayList = 0;
	}

	if (m_optimizedTransparentDiplayList) {
		glDeleteLists(m_optimizedTransparentDiplayList, 1);
		m_optimizedTransparentDiplayList = 0;
	}
}

void DemoMesh::AllocVertexData (int vertexCount)
{
	m_vertexCount = vertexCount;

	m_vertex = new dFloat[3 * m_vertexCount];
	m_normal = new dFloat[3 * m_vertexCount];
	m_uv = new dFloat[2 * m_vertexCount];

	memset (m_vertex, 0, 3 * m_vertexCount * sizeof (dFloat));
	memset (m_normal, 0, 3 * m_vertexCount * sizeof (dFloat));
	memset (m_uv, 0, 2 * m_vertexCount * sizeof (dFloat));
}

DemoSubMesh* DemoMesh::AddSubMesh()
{
	return &Append()->GetInfo();
}


void DemoMesh::Render (DemoEntityManager* const scene)
{
	if (m_isVisible) {
		if (m_optimizedTransparentDiplayList) {
			scene->PushTransparentMesh (this); 
		}

		if (m_optimizedOpaqueDiplayList) {
			glCallList(m_optimizedOpaqueDiplayList);
		} else if (!m_optimizedTransparentDiplayList) {
			glEnableClientState (GL_VERTEX_ARRAY);
			glEnableClientState (GL_NORMAL_ARRAY);
			glEnableClientState (GL_TEXTURE_COORD_ARRAY);

			glVertexPointer (3, GL_FLOAT, 0, m_vertex);
			glNormalPointer (GL_FLOAT, 0, m_normal);
			glTexCoordPointer (2, GL_FLOAT, 0, m_uv);

			for (dListNode* nodes = GetFirst(); nodes; nodes = nodes->GetNext()) {
				DemoSubMesh& segment = nodes->GetInfo();
				segment.Render();
			}
			glDisableClientState(GL_VERTEX_ARRAY);	// disable vertex arrays
			glDisableClientState(GL_NORMAL_ARRAY);	// disable normal arrays
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);	// disable normal arrays
		}
	}
}

void DemoMesh::RenderTransparency () const
{
	if (m_isVisible) {
		if (m_optimizedTransparentDiplayList) {
			glCallList(m_optimizedTransparentDiplayList);
		} else {
			glEnableClientState (GL_VERTEX_ARRAY);
			glEnableClientState (GL_NORMAL_ARRAY);
			glEnableClientState (GL_TEXTURE_COORD_ARRAY);

			glVertexPointer (3, GL_FLOAT, 0, m_vertex);
			glNormalPointer (GL_FLOAT, 0, m_normal);
			glTexCoordPointer (2, GL_FLOAT, 0, m_uv);

			for (dListNode* nodes = GetFirst(); nodes; nodes = nodes->GetNext()) {
				DemoSubMesh& segment = nodes->GetInfo();
				segment.Render();
			}
			glDisableClientState(GL_VERTEX_ARRAY);	// disable vertex arrays
			glDisableClientState(GL_NORMAL_ARRAY);	// disable normal arrays
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);	// disable normal arrays
		}
	}
}

void DemoMesh::RenderNormals ()
{
	glDisable (GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);

	glColor3f(1.0f, 1.0f, 1.0f);

	dFloat length = 0.1f;
	glBegin(GL_LINES);

	for (int i = 0; i < m_vertexCount; i ++) {
		glVertex3f (GLfloat(m_vertex[i * 3 + 0]), GLfloat(m_vertex[i * 3 + 1]), GLfloat(m_vertex[i * 3 + 2]));
		glVertex3f (GLfloat(m_vertex[i * 3 + 0] + m_normal[i * 3 + 0] * length), GLfloat(m_vertex[i * 3 + 1] + m_normal[i * 3 + 1] * length), GLfloat(m_vertex[i * 3 + 2] + m_normal[i * 3 + 2] * length));
	}

	glEnd();

	glEnable (GL_LIGHTING);
}


DemoBezierCurve::DemoBezierCurve(const dScene* const scene, dScene::dTreeNode* const bezierNode)
	:DemoMeshInterface()
	,m_curve()
	,m_renderResolution(50)
{
	m_isVisible = false;
	dLineNodeInfo* const bezeriInfo = (dLineNodeInfo*)scene->GetInfoFromNode(bezierNode);
	dAssert (bezeriInfo->IsType(dLineNodeInfo::GetRttiType()));
	m_name = bezeriInfo->GetName();

	m_curve = bezeriInfo->GetCurve();
}


DemoBezierCurve::DemoBezierCurve (const dBezierSpline& curve)
	:DemoMeshInterface()
	,m_curve(curve)
	,m_renderResolution(50)
{
	m_isVisible = false;
}

void DemoBezierCurve::RenderTransparency () const
{
}

NewtonMesh* DemoBezierCurve::CreateNewtonMesh(NewtonWorld* const world, const dMatrix& meshMatrix)
{
	dAssert(0);
	return NULL;
}

void DemoBezierCurve::RenderNormals ()
{
}	

int DemoBezierCurve::GetRenderResolution () const
{
	return m_renderResolution;
}

void DemoBezierCurve::SetRenderResolution (int breaks)
{
	m_renderResolution = breaks;
}


void DemoBezierCurve::Render (DemoEntityManager* const scene)
{
	if (m_isVisible) {
		glDisable (GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 1.0f);

		dFloat64 scale = 1.0f / m_renderResolution;
		glBegin(GL_LINES);
		dBigVector p0 (m_curve.CurvePoint(0.0f)) ;
		for (int i = 1; i <= m_renderResolution; i ++) {
			dBigVector p1 (m_curve.CurvePoint(i * scale));
			glVertex3f (GLfloat(p0.m_x), GLfloat(p0.m_y), GLfloat(p0.m_z));
			glVertex3f (GLfloat(p1.m_x), GLfloat(p1.m_y), GLfloat(p1.m_z));
			p0 = p1;
		}
		glEnd();
/*
		glPointSize(4.0f);
		glBegin(GL_POINTS);
		glColor3f(1.0f, 0.0f, 0.0f);
		int count = m_curve.GetKnotCount();
		for (int i = 0; i < count; i ++) {
			dFloat u = m_curve.GetKnot(i);
			dBigVector p0 (m_curve.CurvePoint(u));
			glVertex3f (p0.m_x, p0.m_y, p0.m_z);
		}
		glEnd();
*/
		glEnable (GL_LIGHTING);
	}
}

DemoSkinMesh::DemoSkinMesh(dScene* const scene, DemoEntity* const owner, dScene::dTreeNode* const meshNode, const dTree<DemoEntity*, dScene::dTreeNode*>& boneMap)
	:DemoMeshInterface()
	,m_mesh((DemoMesh*)owner->GetMesh())
	,m_root(owner)
	,m_entity(owner)
	,m_vertex(NULL)
	,m_normal(NULL)
	,m_weights(NULL)
	,m_bindingMatrixArray(NULL)
	,m_weighIndex(NULL)
	,m_boneRemapIndex(NULL)
	,m_weightcount(0)
	,m_nodeCount(0)
{
	while (m_root->GetParent()) {
		m_root = m_root->GetParent();
	}
	
	m_mesh->AddRef();
	dMeshNodeInfo* const meshInfo = (dMeshNodeInfo*)scene->GetInfoFromNode(meshNode);
	dAssert (meshInfo->GetTypeId() == dMeshNodeInfo::GetRttiType());

	int boneCount = 0;
	dTree<int, DemoEntity*> nodeEnumrator;
	dTree<DemoEntity*, dScene::dTreeNode*>::Iterator iter (boneMap);
	for (iter.Begin(); iter; iter++) {
		dScene::dTreeNode* const boneNode = iter.GetKey();
		const dGeometryNodeSkinClusterInfo* const cluster = FindSkinModifier(scene, boneNode);
		if (cluster) {
			DemoEntity* const boneEntity = iter.GetNode()->GetInfo();
			boneEntity->SetMatrixUsafe (cluster->m_basePoseMatrix, cluster->m_basePoseMatrix.m_posit);
			boneEntity->SetMatrixUsafe (cluster->m_basePoseMatrix, cluster->m_basePoseMatrix.m_posit);

			nodeEnumrator.Insert(boneCount, boneEntity);
			boneCount ++;
		}
	}

	int stack = 1;
	int entityCount = 0;
	DemoEntity* pool[32];
	dMatrix parentMatrix[32];

	dMatrix* const bindMatrix = dAlloca (dMatrix, 2048);
	DemoEntity** const entityArray = dAlloca (DemoEntity*, 2048);

	m_boneRemapIndex = new int[boneCount];
	memset(m_boneRemapIndex, -1, boneCount * sizeof(int));

	pool[0] = m_root;
	parentMatrix[0] = dGetIdentityMatrix();
	dMatrix shapeBindMatrix(m_entity->GetMeshMatrix() * m_entity->CalculateGlobalMatrix());
	while (stack) {
		stack--;
		DemoEntity* const entity = pool[stack];
		dMatrix boneMatrix(entity->GetCurrentMatrix() * parentMatrix[stack]);

		entityArray[entityCount] = entity;
		bindMatrix[entityCount] = shapeBindMatrix * boneMatrix.Inverse();

		dTree<int, DemoEntity*>::dTreeNode* const mapNode = nodeEnumrator.Find(entity);
		if (mapNode) {
			int index = mapNode->GetInfo();
			dAssert(index < boneCount);
			if (m_boneRemapIndex[index] < 0) {
				m_boneRemapIndex[index] = entityCount;
			}
			dAssert(m_boneRemapIndex[index] == entityCount);
		}

		entityCount++;
		for (DemoEntity* node = entity->GetChild(); node; node = node->GetSibling()) {
			pool[stack] = node;
			parentMatrix[stack] = boneMatrix;
			stack++;
		}
	}

	m_nodeCount = entityCount;
	m_bindingMatrixArray = new dMatrix[entityCount];
	memcpy(m_bindingMatrixArray, bindMatrix, entityCount * sizeof (dMatrix));

	dVector* const weight = new dVector [m_mesh->m_vertexCount];
	dWeightBoneIndex* const skinBone = new dWeightBoneIndex[m_mesh->m_vertexCount];
	memset (weight, 0, m_mesh->m_vertexCount * sizeof (dVector));
	memset (skinBone, -1, m_mesh->m_vertexCount * sizeof (dWeightBoneIndex));

	int vCount = 0;
	for (iter.Begin(); iter; iter++) {
		dScene::dTreeNode* const boneNode = iter.GetKey();
		const dGeometryNodeSkinClusterInfo* const cluster = FindSkinModifier(scene, boneNode);
		if (cluster) {
			DemoEntity* const boneEntity = iter.GetNode()->GetInfo();
			dAssert (nodeEnumrator.Find(boneEntity));
			int boneIndex = nodeEnumrator.Find(boneEntity)->GetInfo();
			for (int i = 0; i < cluster->m_vertexIndex.GetSize(); i ++) {
				int vertexIndex = cluster->m_vertexIndex[i];
				vCount = dMax (vertexIndex, vCount);
				dFloat vertexWeight = cluster->m_vertexWeight[i];
				if (vertexWeight >= weight[vertexIndex][3]) {
					weight[vertexIndex][3] = vertexWeight;
					skinBone[vertexIndex].m_boneIndex[3] = boneIndex;
					for (int j = 2; j >= 0; j --) {
						if (weight[vertexIndex][j] < weight[vertexIndex][j + 1]) {
							dSwap (weight[vertexIndex][j], weight[vertexIndex][j + 1]);
							dSwap (skinBone[vertexIndex].m_boneIndex[j], skinBone[vertexIndex].m_boneIndex[j + 1]);
						}
					}
				}
			}
		}
	}

	m_weightcount = 0;
	for (int i = 0; i < vCount; i ++) {
		dVector w (weight[i]);
		dFloat mag2 = w.m_x * w.m_x + w.m_y * w.m_y + w.m_z * w.m_z + w.m_w * w.m_w;
		mag2 = 1.0f / dSqrt (mag2);
		weight[i].m_x = w.m_x * mag2;
		weight[i].m_y = w.m_y * mag2;
		weight[i].m_z = w.m_z * mag2;
		weight[i].m_w = w.m_w * mag2;

		dAssert (skinBone[i].m_boneIndex[0] != -1);
		for (int j = 0; j < 4; j++) {
			if (skinBone[i].m_boneIndex[j] != -1) {
				m_weightcount = dMax(m_weightcount, j + 1);
			} else {
				skinBone[i].m_boneIndex[j] = 0;
			}
		}
	}

	m_vertex = new dFloat[3 * m_mesh->m_vertexCount];
	m_normal = new dFloat[3 * m_mesh->m_vertexCount];

	m_weights = new dVector[m_mesh->m_vertexCount];
	m_weighIndex = new dWeightBoneIndex[m_mesh->m_vertexCount];
	memset (m_weighIndex, 0, m_mesh->m_vertexCount * sizeof (dWeightBoneIndex));

	const int* const indexToPoitMap = meshInfo->GetIndexToVertexMap();
	for (int i = 0; i < m_mesh->m_vertexCount; i ++) {
//dAssert (i != 6303);
		int index = indexToPoitMap[i];
		dAssert (index >= 0);
		//dAssert (index < vCount);
		if (index < vCount) {
		m_weights[i] = weight[index];
		m_weighIndex[i] = skinBone[index];
		}
	}

	delete[] weight;
	delete[] skinBone;
}

DemoSkinMesh::~DemoSkinMesh()
{
	m_mesh->Release();
	if (m_vertex) {
		delete[] m_vertex;
		delete[] m_normal; 
		delete[] m_weights;
		delete[] m_weighIndex;
		delete[] m_boneRemapIndex;
		delete[] m_bindingMatrixArray; 
	}
}

dGeometryNodeSkinClusterInfo* DemoSkinMesh::FindSkinModifier(dScene* const scene, dScene::dTreeNode* const node) const
{
	for (void* modifierChild = scene->GetFirstChildLink(node); modifierChild; modifierChild = scene->GetNextChildLink(node, modifierChild)) {
		dScene::dTreeNode* const modifierNode = scene->GetNodeFromLink(modifierChild);
		dGeometryNodeSkinClusterInfo* const modifierInfo = (dGeometryNodeSkinClusterInfo*)scene->GetInfoFromNode(modifierNode);
		if (modifierInfo->GetTypeId() == dGeometryNodeSkinClusterInfo::GetRttiType()) {
			return modifierInfo;
		}
	}
	return NULL;
}


void DemoSkinMesh::RenderTransparency () const
{
	m_mesh->RenderTransparency();
}

void DemoSkinMesh::RenderNormals ()
{
	m_mesh->RenderNormals();
}

NewtonMesh* DemoSkinMesh::CreateNewtonMesh(NewtonWorld* const world, const dMatrix& meshMatrix)
{
	return m_mesh->CreateNewtonMesh(world, meshMatrix);
}

void DemoSkinMesh::Render (DemoEntityManager* const scene)
{
	BuildSkin ();

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(3, GL_FLOAT, 0, m_vertex);
	glNormalPointer(GL_FLOAT, 0, m_normal);
	glTexCoordPointer(2, GL_FLOAT, 0, m_mesh->m_uv);

	for (DemoMesh::dListNode* nodes = m_mesh->GetFirst(); nodes; nodes = nodes->GetNext()) {
		DemoSubMesh& segment = nodes->GetInfo();
		segment.Render();
	}
	glDisableClientState(GL_VERTEX_ARRAY);	// disable vertex arrays
	glDisableClientState(GL_NORMAL_ARRAY);	// disable normal arrays
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);	// disable normal arrays
}


void DemoSkinMesh::BuildSkin ()
{

	int stack = 1;
	DemoEntity* pool[32];
	dMatrix parentMatrix[32];
	pool[0] = m_root;
	
	int count = 0;
	dMatrix shapeBindMatrix(m_entity->GetMeshMatrix() * m_entity->CalculateGlobalMatrix());
	shapeBindMatrix = shapeBindMatrix.Inverse();

	parentMatrix[0] = dGetIdentityMatrix();
	dMatrix* const bindMatrix = dAlloca (dMatrix, m_nodeCount);
	while (stack) {
		stack--;
		DemoEntity* const entity = pool[stack];
		dMatrix boneMatrix(entity->GetCurrentMatrix() * parentMatrix[stack]);
		bindMatrix[count] = m_bindingMatrixArray[count] * boneMatrix * shapeBindMatrix;
		count ++;
		dAssert (count <= m_nodeCount);
		for (DemoEntity* node = entity->GetChild(); node; node = node->GetSibling()) {
			pool[stack] = node;
			parentMatrix[stack] = boneMatrix;
			stack++;
		}
	}

	const dFloat* const pointSource = m_mesh->m_vertex;
	const dFloat* const normalSource = m_mesh->m_normal;
	for (int i = 0 ; i < m_mesh->m_vertexCount; i ++) {
		dVector point (pointSource[i * 3 + 0], pointSource[i * 3 + 1], pointSource[i * 3 + 2], dFloat (1.0f));
		dVector normal (normalSource[i * 3 + 0], normalSource[i * 3 + 1], normalSource[i * 3 + 2], dFloat (0.0f));
		dVector weightedPoint (0.0f);
		dVector weightedNormal (0.0f);

		const dVector& weight = m_weights[i];
		const dWeightBoneIndex& boneIndex = m_weighIndex[i];
		for(int j = 0; j < m_weightcount; j ++) {
			int index = m_boneRemapIndex[boneIndex.m_boneIndex[j]];
			dFloat blend = weight[j];
			const dMatrix& matrix = bindMatrix[index];
			weightedPoint += matrix.TransformVector(point).Scale (blend);
			weightedNormal += matrix.RotateVector(normal).Scale (blend);
		}
 		m_vertex[i * 3 + 0] = weightedPoint.m_x;
		m_vertex[i * 3 + 1] = weightedPoint.m_y;
		m_vertex[i * 3 + 2] = weightedPoint.m_z;

		m_normal[i * 3 + 0] = weightedNormal.m_x;
		m_normal[i * 3 + 1] = weightedNormal.m_y;
		m_normal[i * 3 + 2] = weightedNormal.m_z;
	}

//	memcpy(m_vertex, m_mesh->m_vertex, 3 * m_mesh->m_vertexCount * sizeof (dFloat));
//	memcpy(m_normal, m_mesh->m_normal, 3 * m_mesh->m_vertexCount * sizeof (dFloat));
}