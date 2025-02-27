#include "StdAfx.h"

#include "ResourceMesh.h"

#include "GL/glew.h"

#include "Math/float2.h"

#include "Geometry/Triangle.h"

ResourceMesh::ResourceMesh(UID resourceUID,
						   const std::string& fileName,
						   const std::string& assetsPath,
						   const std::string& libraryPath) :
	Resource(resourceUID, fileName, assetsPath, libraryPath),
	vbo(0),
	ebo(0),
	vao(0),
	numVertices(0),
	numFaces(0),
	numIndexes(0),
	numBones(0),
	materialIndex(0)
{
}

ResourceMesh::~ResourceMesh()
{
	Unload();

	bones.clear();
	attaches.clear();
}

unsigned int ResourceMesh::GetBonesPerVertex()
{
	return bonesPerVertex;
}

void ResourceMesh::InternalLoad()
{
	/*glGenBuffers(1, &ssboPalette);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboPalette);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float4x4) * bones.size(), nullptr, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);*/

	CreateVBO();
	CreateEBO();
	CreateVAO();
}

void ResourceMesh::InternalUnload()
{
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
	glDeleteVertexArrays(1, &vao);
	vbo = 0;
	ebo = 0;
	vao = 0;
}

void ResourceMesh::CreateVBO()
{
	if (vbo == 0)
	{
		glGenBuffers(1, &vbo);
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// position			//uv				//normal
	unsigned vertexSize = (sizeof(float) * 3 + sizeof(float) * 2 + sizeof(float) * 3);
	// tangents
	if (tangents.size() != 0)
	{
		vertexSize += sizeof(float) * 3;
	}

	vertexSize += sizeof(unsigned int) * 4;
	vertexSize += sizeof(float) * 4;

	// unsigned vertexSize = (sizeof(float) * 3 + sizeof(float) * 2);
	GLuint bufferSize = vertexSize * numVertices;

	glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_STATIC_DRAW);

	GLuint positionSize = sizeof(float) * 3 * numVertices;

	glBufferSubData(GL_ARRAY_BUFFER, 0, positionSize, &(vertices[0]));

	GLuint uvOffset = positionSize;
	GLuint uvSize = sizeof(float) * 2 * numVertices;

	float2* uvs = (float2*) (glMapBufferRange(GL_ARRAY_BUFFER, uvOffset, uvSize, GL_MAP_WRITE_BIT));

	for (unsigned int i = 0; i < numVertices; ++i)
	{
		uvs[i] = float2(textureCoords[i].x, textureCoords[i].y);
	}

	glUnmapBuffer(GL_ARRAY_BUFFER);

	unsigned normalsOffset = positionSize + uvSize;
	unsigned normalsSize = sizeof(float) * 3 * numVertices;
	glBufferSubData(GL_ARRAY_BUFFER, normalsOffset, normalsSize, &normals[0]);

	unsigned tangentsOffset = positionSize + uvSize + normalsSize;
	unsigned tangentsSize = sizeof(float) * 3 * numVertices;
	if (tangents.size() != 0)
	{
		glBufferSubData(GL_ARRAY_BUFFER, tangentsOffset, tangentsSize, &tangents[0]);
	}

	/*unsigned bonesSize = sizeof(unsigned int) * 4 * numVertices;
	unsigned weightSize = sizeof(float) * 4 * numVertices;
	unsigned boneOffset = positionSize + uvSize + normalsSize + tangentsSize;
	unsigned weightOffset = positionSize + uvSize + normalsSize + tangentsSize + bonesSize;

	typedef struct
	{
		unsigned int x, y, z, w;
	} uint4;

	std::vector<uint4> bones;
	bones.reserve(numVertices);
	std::vector<float4> weights;
	weights.reserve(numVertices);
	for (unsigned int i = 0; i < numVertices; ++i)
	{
		bones.push_back(uint4(attaches[i].bones[0], attaches[i].bones[1], attaches[i].bones[2], attaches[i].bones[3]));

		weights.push_back(
			float4(attaches[i].weights[0], attaches[i].weights[1], attaches[i].weights[2], attaches[i].weights[3]));
	}

	glBufferSubData(GL_ARRAY_BUFFER, boneOffset, bonesSize, &bones[0]);
	glBufferSubData(GL_ARRAY_BUFFER, weightOffset, weightSize, &weights[0]);*/
}

void ResourceMesh::CreateEBO()
{
	if (ebo == 0)
	{
		glGenBuffers(1, &ebo);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

	GLuint indexSize = sizeof(GLuint) * numFaces * 3;

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexSize, nullptr, GL_STATIC_DRAW);

	GLuint* indices = (GLuint*) (glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));

	for (unsigned int i = 0; i < numFaces; ++i)
	{
		assert(facesIndices[i].size() == 3); // note: assume triangles = 3 indices per face
		*(indices++) = facesIndices[i][0];
		*(indices++) = facesIndices[i][1];
		*(indices++) = facesIndices[i][2];
	}

	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}

void ResourceMesh::CreateVAO()
{
	if (vao == 0)
	{
		glGenVertexArrays(1, &vao);
	}

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

	// positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);

	// texCoords
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*) (sizeof(float) * 3 * numVertices));

	// normals
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*) (sizeof(float) * (3 + 2) * numVertices));

	// tangents
	if (tangents.size() != 0)
	{
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*) (sizeof(float) * (3 + 2 + 3) * numVertices));
	}

	//// bone indices and weights
	//glEnableVertexAttribArray(4);
	//glVertexAttribIPointer(4, 4, GL_UNSIGNED_INT, 0, (void*) (sizeof(float) * (3 + 2 + 3 + 3) * numVertices));

	//glEnableVertexAttribArray(5);
	//glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 0,
	//					  (void*) ((sizeof(float) * (3 + 2 + 3 + 3) + sizeof(unsigned int) * 4) * numVertices));
}

// For mouse-picking purposes
const std::vector<Triangle> ResourceMesh::RetrieveTriangles(const float4x4& modelMatrix)
{
	if (!IsLoaded())
	{
		Load();
	}

	// Vertices
	std::vector<float3> vertices;
	vertices.reserve(numVertices);
	for (unsigned i = 0; i < numVertices; ++i)
	{
		// Adapt the mesh vertices to the model matrix of its gameobject transform
		vertices.push_back((modelMatrix.MulPos(this->vertices[i])));
	}

	// Triangles
	std::vector<Triangle> triangles;
	triangles.reserve(numFaces);

	for (unsigned i = 0; i < numFaces; ++i)
	{
		// Retrieve the triangles from the vertices adapted to the model matrix
		triangles.push_back(
			Triangle(vertices[facesIndices[i][0]], vertices[facesIndices[i][1]], vertices[facesIndices[i][2]]));
	}

	return triangles;
}
