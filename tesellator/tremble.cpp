#include "tremble.h"

#ifdef WORK
#include "D:/projects/nIds.h"
#elif HOME
#include "E:/vibgyor/nIds.h"
#endif

#include <maya/MFloatPoint.h>
#include <maya/MFloatVector.h>

#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>

#include <maya/MIntArray.h>

#include <maya/MFnMesh.h>

#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>

#include <testSuite/mayaTestSuite.h>
#include <testSuite/generalSuite.h>

MTypeId tremble::id( nIds::tesellator::nTremble );
MString tremble::nodeName( "tremble" );

MObject tremble::numPassesAttr;
MObject tremble::divisionsAttr;
MObject tremble::bufferDistanceAttr;

MObject tremble::inputMeshAttr;
MObject tremble::collisionObjectsAttr;
MObject tremble::outputMeshAttr;

tremble::tremble(void)
{
}


tremble::~tremble(void)
{
}

void tremble::subdivide( MObject &inputMeshObj, MDataBlock &block )
{
	MArrayDataHandle collisionObjectsHnd = block.inputArrayValue( collisionObjectsAttr );
	
	MIntArray faceIds;

	MFnMesh ipMeshFn( inputMeshObj );

	float bufferDistance = block.inputValue( bufferDistanceAttr ).asFloat();

	for( unsigned int co = 0; co < collisionObjectsHnd.elementCount(); co++ ) {
		collisionObjectsHnd.jumpToArrayElement( co );

		MObject collisionObj = collisionObjectsHnd.inputValue().asMeshTransformed();
		MItMeshVertex polyIt( collisionObj );

		for( polyIt.reset(); !polyIt.isDone(); polyIt.next() ) {
			int index = polyIt.index();

			MVector normal;
			polyIt.getNormal( normal, MSpace::kWorld );
			MPoint center = polyIt.position( MSpace::kWorld );

			MFloatPoint centerF = MFloatPoint( center.x, center.y, center.z );
			MFloatVector normalF( normal );
			
			MFloatPoint hitPnt;
			int hitFace, hitTriangle;
			float hitParam, hitBary1, hitBary2;

			if( ipMeshFn.closestIntersection( centerF, normalF, 0,0,0, MSpace::kWorld, bufferDistance, 0,0, hitPnt, &hitParam, &hitFace, &hitTriangle, &hitBary1, &hitBary2 )) {
				if( !generalSuite::contains( faceIds, hitFace )) {
					faceIds.append( hitFace );
				}
			}
		}
	}

	if( faceIds.length() > 0 ) {
		ipMeshFn.subdivideFaces( faceIds, block.inputValue( divisionsAttr ).asInt() );
	}
}

MStatus tremble::compute( const MPlug &plug, MDataBlock &block )
{
	if( plug == outputMeshAttr ) {
		MObject inputMeshObj = block.inputValue( inputMeshAttr ).asMeshTransformed();

		for( int p = 0; p < block.inputValue( numPassesAttr ).asInt(); p++ ) {
			subdivide( inputMeshObj, block );
		}

		MDataHandle outputHnd = block.outputValue( outputMeshAttr );
		outputHnd.set( inputMeshObj );

		block.setClean( plug );

		return MS::kSuccess;
	}
	else {
		return MS::kUnknownParameter;
	}

	return MS::kSuccess;
}

MStatus tremble::initialize()
{
	MFnNumericAttribute nAttr;

	bufferDistanceAttr = nAttr.create( "bufferDistance", "bf", MFnNumericData::kFloat );
	nAttr.setKeyable( true );
	addAttribute( bufferDistanceAttr );

	divisionsAttr = nAttr.create( "divisions", "d", MFnNumericData::kInt );
	nAttr.setKeyable( true );
	addAttribute( divisionsAttr);

	numPassesAttr = nAttr.create( "numPasses", "np", MFnNumericData::kInt );
	nAttr.setKeyable( true );
	addAttribute( numPassesAttr );

	MFnTypedAttribute tAttr;

	inputMeshAttr = tAttr.create( "inputMesh", "ip", MFnData::kMesh );
	addAttribute( inputMeshAttr );

	outputMeshAttr = tAttr.create( "outputMesh", "op", MFnData::kMesh );
	addAttribute( outputMeshAttr );

	collisionObjectsAttr = tAttr.create( "collisionObjects", "co", MFnData::kMesh );
	tAttr.setArray( true );
	addAttribute( collisionObjectsAttr );

	attributeAffects( bufferDistanceAttr, outputMeshAttr );
	attributeAffects( divisionsAttr, outputMeshAttr );
	attributeAffects( numPassesAttr, outputMeshAttr );
	attributeAffects( inputMeshAttr, outputMeshAttr );
	attributeAffects( collisionObjectsAttr, outputMeshAttr );

	return MS::kSuccess;
}