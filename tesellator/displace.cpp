#include "displace.h"

#ifdef WORK
#include "D:/projects/nIds.h"
#elif HOME
#include "E:/vibgyor/nIds.h"
#endif

#include <maya/MFnNumericAttribute.h>

#include <maya/MItGeometry.h>

#include <maya/MFnMesh.h>

#include <maya/MDataBlock.h>
#include <maya/MRenderUtil.h>

#include <maya/MFloatMatrix.h>

#include <maya/MFloatVector.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFloatPointArray.h>
#include <maya/MPlugArray.h>

#include <testSuite/mayaTestSuite.h>
#include <testSuite/generalSuite.h>

MString displace::nodeName( "displace" );
MObject displace::inputColorAttr;
MObject displace::invertDisplaceAttr;
MObject displace::displaceScaleAttr;
MTypeId displace::id( nIds::tesellator::nDisplace );

displace::displace(void)
{
}


displace::~displace(void)
{
}

MStatus displace::deform( MDataBlock &block, MItGeometry &iter, const MMatrix &mat, unsigned int multiIndex )
{
	MPlug ipColorPlg( thisMObject(), inputColorAttr );
	//MFloatVector c = block.inputValue( inputColorAttr ).asFloatVector();

	if( ipColorPlg.isConnected() ) {
		MPlugArray ipToColor;
		ipColorPlg.connectedTo( ipToColor, true, false );

		MString ipColor = ipToColor[ 0 ].name();

		MArrayDataHandle ipMeshArray = block.inputArrayValue( input );
		ipMeshArray.jumpToArrayElement( multiIndex );

		if( ipMeshArray.inputValue().child( inputGeom ).data().apiType() == MFn::kMeshData ) {
			MFnMesh meshFn( ipMeshArray.inputValue().child( inputGeom ).data() );

			bool invert = block.inputValue( invertDisplaceAttr ).asBool();
			double displaceScale = block.inputValue( displaceScaleAttr ).asDouble();

			float env = block.inputValue( envelope ).asFloat();

			MVectorArray normalArray;
			MFloatArray uArray, vArray;
			MFloatPointArray pArray;
			MFloatVectorArray resultColors, resultTransparencies;
			
			for( iter.reset(); !iter.isDone(); iter.next() ) {
				float weight = weightValue( block, multiIndex, iter.index() );

				MVector normal;
				meshFn.getVertexNormal( iter.index(), true, normal );
				normalArray.append( normal );

				float uv[2];

				meshFn.getUV( iter.index(), uv[ 0 ], uv[ 1 ] );
				uArray.append( uv[ 0 ] ); vArray.append( uv[ 1 ] );

				MPoint pos = iter.position( MSpace::kWorld );

				pArray.append( MFloatPoint( pos.x, pos.y, pos.z, 1 ));
			}

			MFloatMatrix camMatrix;

			MRenderUtil::sampleShadingNetwork( ipColor, pArray.length(), false, false, camMatrix, &pArray, &uArray, &vArray, 0, &pArray, 0,0,0, resultColors, resultTransparencies );

			for( iter.reset(); !iter.isDone(); iter.next() ) {
				int index = iter.index();
				float amt = resultColors[ index ].x * 0.299 + resultColors[ index ].y * 0.587 + resultColors[ index ].z * 0.114;

				MPoint displacedPnt = MPoint( amt * normalArray[ index ].x, amt * normalArray[ index ].y, amt * normalArray[ index ].z, 1 );
				MPoint scaledPnt = displacedPnt * displaceScale;

				if( invert ) {
					iter.setPosition( iter.position() + MPoint( -scaledPnt.x, -scaledPnt.y, -scaledPnt.z, 1 ));
				}
				else {
					iter.setPosition( iter.position() + scaledPnt );
				}
			}
		}
	}
	else {
	}

	return MS::kSuccess;
}

MStatus displace::initialize()
{
	MFnNumericAttribute nAttr;
	
	invertDisplaceAttr = nAttr.create( "invertDisplace", "id", MFnNumericData::kBoolean, false );
	nAttr.setKeyable( true );
	addAttribute( invertDisplaceAttr );

	inputColorAttr = nAttr.createColor( "color", "ic" );
	nAttr.setStorable( false );
	addAttribute( inputColorAttr );

	displaceScaleAttr = nAttr.create( "displaceScale", "ds", MFnNumericData::kDouble, 1 );
	nAttr.setMin( 0 );
	nAttr.setKeyable( true );
	addAttribute( displaceScaleAttr );

	attributeAffects( inputColorAttr, outputGeom );
	attributeAffects( invertDisplaceAttr, outputGeom );
	attributeAffects( displaceScaleAttr, outputGeom );

	return MS::kSuccess;
}