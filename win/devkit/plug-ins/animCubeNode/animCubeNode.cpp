//-
// ==========================================================================
// Copyright 1995,2006,2008 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
// ==========================================================================
//+

#include <maya/MTime.h>
#include <maya/MFnMesh.h>
#include <maya/MPoint.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatPointArray.h>
#include <maya/MIntArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnPlugin.h>

#include <maya/MPxNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MFnMeshData.h>

#include <maya/MIOStream.h>

MStatus returnStatus;

#define McheckErr(stat,msg)			\
	if ( MS::kSuccess != stat ) {	\
		cerr << msg;				\
		return MS::kFailure;		\
	}

class animCube : public MPxNode
{
public:
					animCube() {};
	virtual 		~animCube() {};
	virtual MStatus compute(const MPlug& plug, MDataBlock& data);
	static  void*	creator();
	static  MStatus initialize();

	static MObject	time;
	static MObject	outputMesh;
	static MTypeId	id;

protected:
	MObject createMesh(const MTime& time, MObject& outData, MStatus& stat);
};

MObject animCube::time;
MObject animCube::outputMesh;
MTypeId animCube::id( 0x80000 );

void* animCube::creator()
{
	return new animCube;
}

MStatus animCube::initialize()
{
	MFnUnitAttribute unitAttr;
	MFnTypedAttribute typedAttr;

	MStatus returnStatus;

	animCube::time = unitAttr.create( "time", "tm",
										  MFnUnitAttribute::kTime,
										  0.0, &returnStatus );
	McheckErr(returnStatus, "ERROR creating animCube time attribute\n");


	animCube::outputMesh = typedAttr.create( "outputMesh", "out",
												 MFnData::kMesh,
												 &returnStatus ); 
	McheckErr(returnStatus, "ERROR creating animCube output attribute\n");
	typedAttr.setStorable(false);

	returnStatus = addAttribute(animCube::time);
	McheckErr(returnStatus, "ERROR adding time attribute\n");

	returnStatus = addAttribute(animCube::outputMesh);
	McheckErr(returnStatus, "ERROR adding outputMesh attribute\n");

	returnStatus = attributeAffects(animCube::time,
								    animCube::outputMesh);
	McheckErr(returnStatus, "ERROR in attributeAffects\n");

	return MS::kSuccess;
}

MObject animCube::createMesh(const MTime& time,
							  MObject& outData,
							  MStatus& stat)

{
	int				numVertices, frame;
	float			cubeSize;
	MFloatPointArray		points;
	MFnMesh			meshFS;

	// Scale the cube on the frame number, wrap every 10 frames.
	frame = (int)time.as( MTime::kFilm );
	if (frame == 0)
	  frame = 1;
	cubeSize		    		= 0.5f * (float)( frame % 10);

	const int numFaces			= 6;
	numVertices					= 8;
	const int numFaceConnects	= 24;

	MFloatPoint vtx_1( -cubeSize, -cubeSize, -cubeSize );
	MFloatPoint vtx_2(  cubeSize, -cubeSize, -cubeSize );
	MFloatPoint vtx_3(  cubeSize, -cubeSize,  cubeSize );
	MFloatPoint vtx_4( -cubeSize, -cubeSize,  cubeSize );
	MFloatPoint vtx_5( -cubeSize,  cubeSize, -cubeSize );
	MFloatPoint vtx_6( -cubeSize,  cubeSize,  cubeSize );
	MFloatPoint vtx_7(  cubeSize,  cubeSize,  cubeSize );
	MFloatPoint vtx_8(  cubeSize,  cubeSize, -cubeSize );
	points.append( vtx_1 );
	points.append( vtx_2 );
	points.append( vtx_3 );
	points.append( vtx_4 );
	points.append( vtx_5 );
	points.append( vtx_6 );
	points.append( vtx_7 );
	points.append( vtx_8 );

	// Set up an array containing the number of vertices
	// for each of the 6 cube faces (4 verticies per face)
	//
	int face_counts[numFaces] = { 4, 4, 4, 4, 4, 4 };
	MIntArray faceCounts( face_counts, numFaces );

	// Set up and array to assign vertices from points to each face 
	//
	int face_connects[ numFaceConnects ] = {	0, 1, 2, 3,
												4, 5, 6, 7,
												3, 2, 6, 5,
												0, 3, 5, 4,
												0, 4, 7, 1,
												1, 7, 6, 2	};
	MIntArray faceConnects( face_connects, numFaceConnects );

	MObject newMesh = meshFS.create(numVertices, numFaces,
									points, faceCounts, faceConnects,
									outData, &stat);

	return newMesh;
}

MStatus animCube::compute(const MPlug& plug, MDataBlock& data)

{
	MStatus returnStatus;

	if (plug == outputMesh) {
		/* Get time */
		MDataHandle timeData = data.inputValue( time, &returnStatus ); 
		McheckErr(returnStatus, "Error getting time data handle\n");
		MTime time = timeData.asTime();

		/* Get output object */

		MDataHandle outputHandle = data.outputValue(outputMesh, &returnStatus);
		McheckErr(returnStatus, "ERROR getting polygon data handle\n");

		MFnMeshData dataCreator;
		MObject newOutputData = dataCreator.create(&returnStatus);
		McheckErr(returnStatus, "ERROR creating outputData");

		createMesh(time, newOutputData, returnStatus);
		McheckErr(returnStatus, "ERROR creating new Cube");

		outputHandle.set(newOutputData);
		data.setClean( plug );
	} else
		return MS::kUnknownParameter;

	return MS::kSuccess;
}

MStatus initializePlugin(MObject obj)
{
	MStatus   status;
	MFnPlugin plugin(obj, PLUGIN_COMPANY, "3.0", "Any");

	status = plugin.registerNode("animCube", animCube::id,
						 animCube::creator, animCube::initialize);
	if (!status) {
		status.perror("registerNode");
		return status;
	}

	return status;
}

MStatus uninitializePlugin(MObject obj)
{
	MStatus	  status;
	MFnPlugin plugin(obj);

	status = plugin.deregisterNode(animCube::id);
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}
