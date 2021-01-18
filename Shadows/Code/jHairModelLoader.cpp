#include "pch.h"
#include "jHairModelLoader.h"
#include "External/cyHair/cyHairFile.h"
#include "Math/Vector.h"
#include "jMeshObject.h"
#include "jRenderObject.h"

jHairModelLoader* jHairModelLoader::_instance = nullptr;

jHairModelLoader::jHairModelLoader()
{
}


jHairModelLoader::~jHairModelLoader()
{
}

jHairObject* jHairModelLoader::CreateHairObject(const char* filename)
{
	cyHairFile hairFile;

	int result = hairFile.LoadFromFile(filename);
	// Check for errors
	switch (result)
	{
	case CY_HAIR_FILE_ERROR_CANT_OPEN_FILE:
		JASSERT(!"Error: Cannot open hair file!\n");
		break;
	case CY_HAIR_FILE_ERROR_CANT_READ_HEADER:
		JASSERT(!"Error: Cannot read hair file header!\n");
		break;
	case CY_HAIR_FILE_ERROR_WRONG_SIGNATURE:
		JASSERT(!"Error: File has wrong signature!\n");
		break;
	case CY_HAIR_FILE_ERROR_READING_SEGMENTS:
		JASSERT(!"Error: Cannot read hair segments!\n");
		break;
	case CY_HAIR_FILE_ERROR_READING_POINTS:
		JASSERT(!"Error: Cannot read hair points!\n");
		break;
	case CY_HAIR_FILE_ERROR_READING_COLORS:
		JASSERT(!"Error: Cannot read hair colors!\n");
		break;
	case CY_HAIR_FILE_ERROR_READING_THICKNESS:
		JASSERT(!"Error: Cannot read hair thickness!\n");
		break;
	case CY_HAIR_FILE_ERROR_READING_TRANSPARENCY:
		JASSERT(!"Error: Cannot read hair transparency!\n");
		break;
	};

	int hairCount = hairFile.GetHeader().hair_count;
	int pointCount = hairFile.GetHeader().point_count;
	float *dirs = new float[pointCount * 3];
	
	Vector4 hairColor;
	hairColor.x = hairFile.GetHeader().d_color[0];
	hairColor.y = hairFile.GetHeader().d_color[1];
	hairColor.z = hairFile.GetHeader().d_color[2];
	hairColor.w = hairFile.GetHeader().d_transparency;

	// Compute directions
	if (!hairFile.FillDirectionArray(dirs))
		JASSERT(!"Error: Cannot compute hair directions!\n");

	// Draw arrays
	int pointIndex = 0;
	const unsigned short *segments = hairFile.GetSegmentsArray();
	float *points = hairFile.GetPointsArray();

	int32 numOfVertex = hairFile.GetHeader().point_count * 4;
	Vector* vertexData = new Vector[numOfVertex];
	Vector* normalData = new Vector[numOfVertex];

	if (segments)
	{
		// If segments array exists
		for (int hairIndex = 0; hairIndex < hairCount; hairIndex++)
		{
			for (int point = pointIndex; point < pointIndex + segments[hairIndex]; point++)
			{
				vertexData[point * 2 + 0].x = points[point * 3 + 0];
				vertexData[point * 2 + 0].z = points[point * 3 + 1];
				vertexData[point * 2 + 0].y = points[point * 3 + 2];

				normalData[point * 2 + 0].x = dirs[point * 3 + 0];
				normalData[point * 2 + 0].z = dirs[point * 3 + 1];
				normalData[point * 2 + 0].y = dirs[point * 3 + 2];

				vertexData[point * 2 + 1].x = points[(point + 1) * 3 + 0];
				vertexData[point * 2 + 1].z = points[(point + 1) * 3 + 1];
				vertexData[point * 2 + 1].y = points[(point + 1) * 3 + 2];

				normalData[point * 2 + 1].x = dirs[(point + 1) * 3 + 0];
				normalData[point * 2 + 1].z = dirs[(point + 1) * 3 + 1];
				normalData[point * 2 + 1].y = dirs[(point + 1) * 3 + 2];
			}
			pointIndex += segments[hairIndex] + 1;
		}
	}
	else
	{
		// If segments array does not exist, use default segment count
		int dsegs = hairFile.GetHeader().d_segments;
		for (int hairIndex = 0; hairIndex < hairCount; hairIndex++)
		{
			for (int point = pointIndex; point < pointIndex + dsegs; point++)
			{
				vertexData[point * 2 + 0].x = points[point * 3 + 0];
				vertexData[point * 2 + 0].z = points[point * 3 + 1];
				vertexData[point * 2 + 0].y = points[point * 3 + 2];

				normalData[point * 2 + 0].x = dirs[point * 3 + 0];
				normalData[point * 2 + 0].z = dirs[point * 3 + 1];
				normalData[point * 2 + 0].y = dirs[point * 3 + 2];

				vertexData[point * 2 + 1].x = points[(point + 1) * 3 + 0];
				vertexData[point * 2 + 1].z = points[(point + 1) * 3 + 1];
				vertexData[point * 2 + 1].y = points[(point + 1) * 3 + 2];

				normalData[point * 2 + 1].x = dirs[(point + 1) * 3 + 0];
				normalData[point * 2 + 1].z = dirs[(point + 1) * 3 + 1];
				normalData[point * 2 + 1].y = dirs[(point + 1) * 3 + 2];
			}

			pointIndex += dsegs + 1;
		}
	}

	// attribute 추가
	auto vertexStreamData = std::shared_ptr<jVertexStreamData>(new jVertexStreamData());

	{
		auto streamParam = new jStreamParam<float>();
		streamParam->BufferType = EBufferType::STATIC;
		streamParam->ElementTypeSize = sizeof(float);
		streamParam->ElementType = EBufferElementType::FLOAT;
		streamParam->Stride = sizeof(float) * 3;
		streamParam->Name = "Pos";
		streamParam->Data.resize(numOfVertex * 3);
		memcpy(&streamParam->Data[0], vertexData, numOfVertex * sizeof(Vector));
		vertexStreamData->Params.push_back(streamParam);
	}

	{
		auto streamParam = new jStreamParam<float>();
		streamParam->BufferType = EBufferType::STATIC;
		streamParam->ElementTypeSize = sizeof(float);
		streamParam->ElementType = EBufferElementType::FLOAT;
		streamParam->Stride = sizeof(float) * 3;
		streamParam->Name = "Normal";
		streamParam->Data.resize(numOfVertex * 3);
		memcpy(&streamParam->Data[0], normalData, numOfVertex * sizeof(Vector));
		vertexStreamData->Params.push_back(streamParam);
	}

	vertexStreamData->PrimitiveType = EPrimitiveType::LINES;
	vertexStreamData->ElementCount = numOfVertex;

	auto object = new jHairObject();
	object->RenderObject = new jRenderObject();
	object->RenderObject->CreateRenderObject(vertexStreamData, nullptr);
	object->RenderObject->Color = hairColor;
	object->RenderObject->UseUniformColor = 1;
	object->RenderObject->ShadingModel = EShadingModel::HAIR;

	delete vertexData;
	delete normalData;
	delete dirs;

	return object;
}
