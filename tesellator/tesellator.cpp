#include "tesellator.h"
#ifdef WORK
#include "D:/projects/nIds.h"
#elif HOME
#include "e:/vibgyor/nIds.h"
#endif

#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>

#include <maya/MFnMesh.h>
#include <maya/MFnMeshData.h>
#include <maya/MImage.h>
#include <maya/MGlobal.h>
#include <maya/MFileObject.h>
#include <maya/MDagPath.h>
#include <maya/MRenderUtil.h>
#include <maya/MFloatMatrix.h>

#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>

#include <maya/MStringArray.h>
#include <maya/MPointArray.h>
#include <maya/MPlugArray.h>
#include <maya/MFloatPointArray.h>

#include <maya/MTime.h>

#include <testSuite/mayaTestSuite.h>
#include <testSuite/generalSuite.h>

using namespace std;

MObject tesellator::colorThresholdAttr;
MObject tesellator::divisionAttr;
MObject tesellator::numPassesAttr;
MObject tesellator::inputMeshAttr;

MObject tesellator::inverseAttr;

MObject tesellator::resolutionAttr;

MObject tesellator::inputColorAttr;
MObject tesellator::outputMeshAttr;

MTypeId tesellator::id( nIds::tesellator::nTesellator );
MString tesellator::nodeName( "tesellator" );

tesellator::tesellator(void)
{
}

tesellator::~tesellator(void)
{
}

void addMapData( MImage *D, MImage *d )
{
#ifdef VERBOSE
	cout << "addMapData" << endl;
#endif

	unsigned char *dPixels = d->pixels();
	unsigned char *DPixels = D->pixels();

	unsigned int dHeight, dWidth, dDepth;
	unsigned int DHeight, DWidth, DDepth;

	d->getSize( dWidth, dHeight ); D->getSize( DWidth, DHeight );
	dDepth = d->depth(); DDepth = D->depth();

	float hRatio = (float) dHeight / (float) DHeight;
	float wRatio = (float) dWidth / (float) DWidth;

	if( DHeight != dHeight || DWidth != dWidth || dDepth != DDepth ) {
		MGlobal::displayWarning( "Image sizes and channels do not match" );
	}

	for( unsigned int y = 0; y < DHeight; y++ ) {
		for( unsigned int x = 0; x < DWidth; x++ ) {

			int newX = x * wRatio; int newY = y * hRatio;

			DPixels[ ( y * DWidth + x ) * DDepth ] += ( dPixels[ ( newY * dWidth + newX ) * dDepth ] );

			if( DPixels[ ( y * DWidth + x ) * DDepth ] > 255 ) {
				DPixels[ ( y * DWidth + x ) * DDepth ] = 255;
			}

			DPixels[ ( y * DWidth + x ) * DDepth + 1 ] += ( dPixels[ ( newY * dWidth + newX ) * dDepth + 1 ] );

			if( DPixels[ ( y * DWidth + x ) * DDepth + 1 ] > 255 ) {
				DPixels[ ( y * DWidth + x ) * DDepth + 1 ] = 255;
			}

			DPixels[ ( y * DWidth + x ) * DDepth + 2 ] += ( dPixels[ ( newY * dWidth + newX ) * dDepth + 2 ] );

			if( DPixels[ ( y * DWidth + x ) * DDepth + 2 ] > 255 ) {
				DPixels[ ( y * DWidth + x ) * DDepth + 2 ] = 255;
			}
		}
	}
}

void grey( MImage *img )
{
#ifdef VERBOSE
	cout << "grey" << endl;
#endif

	unsigned char *data = img->pixels();

	unsigned int w,h,d;

	img->getSize( w,h ); d = img->depth();

	for( unsigned int y = 0; y < h; y++ ) {
		for( unsigned int x = 0; x < w; x++ ) {
			unsigned char tmp[3];
			int offset = y * w + x;

			tmp[0] = data[ offset * d ]; tmp[1] = data[ offset * d + 1 ]; tmp[2] = data[ offset * d + 2 ];

			data[ offset * d ] = ( tmp[ 0 ] * 0.299 ) + ( tmp[ 1 ] * 0.587 ) + ( tmp[ 2 ] * 0.114 );
			data[ offset * d + 1 ] = ( tmp[ 0 ] * 0.299 ) + ( tmp[ 1 ] * 0.587 ) + ( tmp[ 2 ] * 0.114 );
			data[ offset * d + 2 ] = ( tmp[ 0 ] * 0.299 ) + ( tmp[ 1 ] * 0.587 ) + ( tmp[ 2 ] * 0.114 );
		}
	}
}

void evenOut( MObject &inputMeshObj )
{
#ifdef VERBOSE
	cout << "evenOut" << endl;
#endif

	MIntArray faceIds;

	MItMeshPolygon polyIt( inputMeshObj );
	MFnMesh meshFn( inputMeshObj );

	MPointArray meshPoints;

	for( polyIt.reset(); !polyIt.isDone(); polyIt.next() ) {
		int numVertices = polyIt.polygonVertexCount();

		if( numVertices > 4 ) {
			MPointArray points;
			polyIt.getPoints( points );

			//MPoint avgPoint = points[0];

			//{
			//	for( int i = 1; i < points.length(); i++ ) {
			//		avgPoint += points[ i ];
			//	}
			//}

			//avgPoint = avgPoint / points.length();
			//
			//points.append( avgPoint );

			{
				meshFn.deleteFace( polyIt.index() );
				int magicNumber = ( numVertices % 2 ) ? numVertices / 2 : numVertices / 2 - 1;

				for( int i = 0; i < points.length() - 1; i++ ) {
					if( i != magicNumber ) {
						MPointArray newPoints;
						newPoints.append( points[ i ] );
						newPoints.append( points[ i + 1 ] );
						newPoints.append( points[ numVertices - 1 - i ] );

						meshFn.addPolygon( newPoints, true, 0.1, inputMeshObj );						
					}
				}
			}
		}
	}

	polyIt.updateSurface();
}

void displace( MObject &inputMeshObj, MImage &dMapsImage, double displaceScale, bool invert )
{
#ifdef VERBOSE
	cout << "displace" << endl;
#endif

	MItMeshVertex vertIt( inputMeshObj );
	
	MVectorArray normalArray;
	normalArray.setLength( vertIt.count() );

	for( vertIt.reset(); !vertIt.isDone(); vertIt.next() ) {
		MVector normal;
		vertIt.getNormal( normal );
		normalArray[ vertIt.index() ] = normal;
	}

	unsigned int w,h, d = dMapsImage.depth();
	dMapsImage.getSize( w,h );

	unsigned char *dMapsData = dMapsImage.pixels();

	for( vertIt.reset(); !vertIt.isDone(); vertIt.next() ) {
		int index = vertIt.index();

		float uv[2];
		vertIt.getUV( uv );

		int x = (int)( uv[0] * w ) == w ? ( w - 1 ) : ( uv[0] * w );
		int y = (int)( uv[1] * h ) == h ? ( h - 1 ) : ( uv[1] * h );

		int offset = y * w + x;

		double amt = dMapsData[ offset * d ];

		amt /= 255.f;

		MPoint displacedPnt = MPoint( amt * normalArray[ index ].x, amt * normalArray[ index ].y, amt * normalArray[ index ].z );
		MPoint scaledPnt = displacedPnt * displaceScale;

		if( invert ) {
			vertIt.translateBy( MPoint( -scaledPnt.x, -scaledPnt.y, -scaledPnt.z, scaledPnt.w ));
		}
		else {
			vertIt.translateBy( scaledPnt );
		}
	}
}

void compileDMaps( MImage *dMapsImage, MStringArray dMapPathArray )
{
#ifdef VERBOSE
	cout << "compileDMaps" << endl;
#endif

	for( unsigned int m = 0; m < dMapPathArray.length(); m++ ) {
		MString dMapPath = dMapPathArray[ m ];
		MFileObject mapObject;
		mapObject.setRawFullName( dMapPath );

		if( !mapObject.exists() ) {
			MGlobal::displayError( dMapPath + MString( " does not exist." ));
			cout << dMapPath.asChar() << " does not exist" << endl;
			continue;
		}

		MImage img;
		img.readFromFile( dMapPath );
				
		if( m == 0 ) {
			dMapsImage->readFromFile( dMapPath );
		}

		if( m > 0 ) {
			addMapData( dMapsImage, &img );
		}
	}

	grey( dMapsImage );
}

void getTriFaceIds( MIntArray &faceIds, MImage *dMapsImage, MFloatArray uArray, MFloatArray vArray, int resolution, int colorThreshold, int faceIndex, int t )
{
#ifdef VERBOSE
	cout << "getTriFaceIds" << endl;
#endif

	unsigned char *dMapsData = dMapsImage->pixels();

	unsigned char lastColor;

	for( float u = 0; u <= 1.0; u += 1.f / resolution ) {
		for( float v = 0; v <= 1.0; v += 1.f / resolution ) {
			float currentU, currentV;

			if( t == 0 ) {
				currentU = (( 1 - u - v ) * uArray[ 0 ] ) + ( u * uArray[ 1 ] ) + ( v * uArray[ 3 ] );
				currentV = (( 1 - u - v ) * vArray[ 0 ] ) + ( u * vArray[ 1 ] ) + ( v * vArray[ 3 ] );
			}
			else {
				currentU = (( 1 - u - v ) * uArray[ 3 ] ) + ( u * uArray[ 1 ] ) + ( v * uArray[ 2 ] );
				currentV = (( 1 - u - v ) * vArray[ 3 ] ) + ( u * vArray[ 1 ] ) + ( v * vArray[ 2 ] );
			}

			unsigned int w, h, d = dMapsImage->depth();
			dMapsImage->getSize( w, h );

			int x = ( currentU * w ) == w ? ( w - 1 ) : ( currentU * w );
			int y = ( currentV * h ) == h ? ( h - 1 ) : ( currentV * h );

			int offset = y * w + x;

			if(( u && v ) == 0 ) {
				lastColor = dMapsData[ offset * d ];
			}

			unsigned char threshold = colorThreshold;

			if( lastColor - dMapsData[ offset * d ] > threshold ) {
				if( !generalSuite::contains( faceIds, faceIndex )) {
					faceIds.append( faceIndex );
					return;
				}
			}

			lastColor = dMapsData[ offset * d ];
		}
	}
}

void createFilePathArray( MArrayDataHandle hnd, MStringArray &pathArray )
{
#ifdef VERBOSE
	cout << "createFilePathArray" << endl;
#endif

	unsigned int numDMaps = hnd.elementCount();

	for( unsigned int m = 0; m < numDMaps; m++ ) {
		hnd.jumpToArrayElement( m );

		MString finalPath = hnd.inputValue().asString();

		MFileObject fileObject;
		fileObject.setRawFullName( finalPath );

		if( !fileObject.exists() ) {
			MGlobal::displayError( finalPath + " does not exist." );
			cout << finalPath.asChar() << " does exist." << endl;
			continue;
		}

		pathArray.append( finalPath );
	}
}

void createFilePathArray( MArrayDataHandle hnd, MStringArray &pathArray, int frameAfterOffset )
{
#ifdef VERBOSE
	cout << "createFilePathArray( offset )" << endl;
#endif

	unsigned int numDMaps = hnd.elementCount();
	
	for( unsigned int m = 0; m < numDMaps; m++ ) {
		hnd.jumpToArrayElement( m );

		MString basePath = hnd.inputValue().asString();
		
		MString cmd = MString( "dirname( \"" ) + basePath + "\" )";
        MString dirName, fileName;
        MGlobal::executeCommand( cmd, dirName );

        cmd = MString( "basenameEx( \"" ) + basePath + "\" )";
        MGlobal::executeCommand( cmd, fileName );

		MStringArray basePathArray; basePath.split( '/', basePathArray );
		MStringArray fileNameArray; basePathArray[ basePathArray.length() - 1 ].split( '.', fileNameArray );

		char paddedFrame[10];

		int padLength = fileNameArray[ 1 ].length();

		if( padLength == 4 ) {
			sprintf( paddedFrame, "%04d", frameAfterOffset );
		}
		else if( padLength == 5 ) {
			sprintf( paddedFrame, "%05d", frameAfterOffset );
		}
		else if( padLength == 6 ) {
			sprintf( paddedFrame, "%06d", frameAfterOffset );
		}
		else {
			MGlobal::displayError( "Padding of 4 / 5 / 6 supported by plugin. " );
			continue;
		}

		MString finalPath = dirName + "/" + fileNameArray[ 0 ] + "." + ( paddedFrame ) + "." + fileNameArray[ 2 ];

		MFileObject fileObject;
		fileObject.setRawFullName( finalPath );

		if( !fileObject.exists() ) {
			MGlobal::displayError( finalPath + " does not exist." );
			cout << finalPath.asChar() << " does not exist." << endl;
			continue;
		}

		pathArray.append( finalPath );
	}
}

void getTriFaceIds( MIntArray &faceIds, MString ipToColor, MFloatArray uArray, MFloatArray vArray, MFloatPointArray pArray, int resolution, float threshold, int faceIndex, int t )
{
	MFloatArray tmpU, tmpV;
	MFloatPointArray tmpP;

	for( float u = 0; u <= 1.0; u += 1.f / resolution ) {
		for( float v = 0; v <= 1.0; v += 1.f / resolution ) {
			float currentU, currentV;
			MFloatPoint currentP;

			if( t == 0 ) {
				currentU = (( 1 - u - v ) * uArray[ 0 ] ) + ( u * uArray[ 1 ] ) + ( v * uArray[ 3 ] );
				currentV = (( 1 - u - v ) * vArray[ 0 ] ) + ( u * vArray[ 1 ] ) + ( v * vArray[ 3 ] );

				currentP =  pArray[ 0 ] * ( 1 - u - v ) + ( pArray[ 1 ] * u ) + ( pArray[ 3 ] );
			}
			else {
				currentU = (( 1 - u - v ) * uArray[ 3 ] ) + ( u * uArray[ 1 ] ) + ( v * uArray[ 2 ] );
				currentV = (( 1 - u - v ) * vArray[ 3 ] ) + ( u * vArray[ 1 ] ) + ( v * vArray[ 2 ] );
				
				currentP =  pArray[ 3 ] * ( 1 - u - v ) + ( pArray[ 1 ] * u ) + ( pArray[ 2 ] );
			}

			tmpU.append( currentU ); tmpV.append( currentV ); tmpP.append( currentP );
		}
	}
	
	MFloatVectorArray resultColors, resultTransparencies;
	MFloatMatrix camMatrix; camMatrix.setToIdentity();

	MRenderUtil::sampleShadingNetwork( ipToColor, tmpU.length(), false, false, camMatrix, &tmpP, &tmpU, &tmpV, 0, &tmpP ,0,0,0, resultColors, resultTransparencies );

	if( resultColors.length() > 0  ) {
		float lastGrey = resultColors[ 0 ].x * 0.299 + resultColors[ 0 ].y * 0.587 + resultColors[ 0 ].z * 0.114;

		for( unsigned int i = 1; i < resultColors.length(); i++ ) {
			float currentGrey = resultColors[ i ].x * 0.299 + resultColors[ i ].y * 0.587 + resultColors[ i ].z * 0.114;

			if(( lastGrey - currentGrey ) > threshold ) {
				if( !generalSuite::contains( faceIds, faceIndex )) {
					faceIds.append( faceIndex );
					return;
				}
			}
			
			lastGrey = currentGrey;
		}
	}
}

void tesellator::subdivide( MObject &inputMeshObj, MDataBlock &block, MImage *dMapsImage )
{
#ifdef VERBOSE
	cout << "tesellator::subdivide" << endl;
#endif

	int colorThreshold = block.inputValue( colorThresholdAttr ).asInt();
	int resolution = block.inputValue( resolutionAttr ).asInt();

	MItMeshPolygon polyIt( inputMeshObj );
	MIntArray faceIds;

	for( polyIt.reset(); !polyIt.isDone(); polyIt.next() ) {
		MFloatArray uArray, vArray;

		polyIt.getUVs( uArray, vArray );

		if( uArray.length() != vArray.length() ) {
			MGlobal::displayError( "varying amounts of u and v. You are great!" );
		}

		if( uArray.length() == 3 ) {
			//barycentric code

			getTriFaceIds( faceIds, dMapsImage, uArray, vArray, resolution, colorThreshold, polyIt.index(), 0 );

		}
		else if( uArray.length() == 4 ) {
			/*
				3---2
				|\  |
				|  \|
				0---1
				013 and 312 go into barycentric code
				( 1 - u - v )P0 - uP1 - vP2
			*/

			for( int t = 0; t < 2; t ++ ) {
				getTriFaceIds( faceIds, dMapsImage, uArray, vArray, resolution, colorThreshold, polyIt.index(), t );
			}
		}
	}

	if( faceIds.length() > 0 ) {
		MFnMesh meshFn( inputMeshObj );
		meshFn.subdivideFaces( faceIds, block.inputValue( divisionAttr ).asInt() );
	}

	faceIds.clear();
}

void tesellator::subdivide( MObject &inputMeshObj, MDataBlock &block, MString ipToColor )
{
	MItMeshPolygon polyIt( inputMeshObj );
	float colorThreshold = block.inputValue( colorThresholdAttr ).asFloat();
	int resolution = block.inputValue( resolutionAttr ).asInt();

	MIntArray faceIds;

	for( polyIt.reset(); !polyIt.isDone(); polyIt.next() ) {
		MFloatArray uArray, vArray;

		polyIt.getUVs( uArray, vArray );

		MPointArray tmpArray; polyIt.getPoints( tmpArray, MSpace::kWorld );
		MFloatPointArray pArray;

		for( unsigned int i = 0; i < tmpArray.length(); i++ ) {
			pArray.append( MFloatPoint( tmpArray[ i ].x, tmpArray[ i ].y, tmpArray[ i ].z, tmpArray[ i ].w ));
		}
		
		if( uArray.length() != vArray.length() ) {
			MGlobal::displayError( "varying amounts of u and v. There is greatness and then there is You!");
		}

		if( uArray.length() == 3 ) {
			getTriFaceIds( faceIds, ipToColor, uArray, vArray, pArray, resolution, colorThreshold, polyIt.index(), 0 );
		}
		else if( uArray.length() == 4 ) {
			for( int t = 0; t < 2; t ++ ) {
				getTriFaceIds( faceIds, ipToColor, uArray, vArray, pArray, resolution, colorThreshold, polyIt.index(), t );
			}
		}
	}

	if( faceIds.length() > 0 ) {
		MFnMesh meshFn( inputMeshObj );
		meshFn.subdivideFaces( faceIds, block.inputValue( divisionAttr ).asInt() );
	}

	faceIds.clear();
}

MStatus tesellator::compute( const MPlug &plug, MDataBlock &block )
{
#ifdef VERBOSE
	cout << "tesellator::compute" << endl;
#endif
	
	if( plug == outputMeshAttr ) {
		MObject inputMeshObj = block.inputValue( inputMeshAttr ).asMeshTransformed();
		
		MPlug ipColorPlug( thisMObject(), inputColorAttr );
		MFloatVector c = block.inputValue( inputColorAttr ).asFloatVector();

		if( ipColorPlug.isConnected() ) {
			MPlugArray ipToColor;
			ipColorPlug.connectedTo( ipToColor, true, false );

			MString ipColor = ipToColor[ 0 ].name();
			
			int numPasses = block.inputValue( numPassesAttr ).asInt();

			for( int p = 0; p < numPasses; p++ ) {
				subdivide( inputMeshObj, block, ipColor );
				evenOut( inputMeshObj );
			}
		}
		else {
		}
		
		MDataHandle outHnd = block.outputValue( outputMeshAttr );
		outHnd.set( inputMeshObj );

		block.setClean( plug );

		return MS::kSuccess;
	}
	else {
		return MS::kUnknownParameter;
	}

	return MS::kSuccess;
}

MStatus tesellator::initialize()
{
	MFnTypedAttribute tAttr;

	inputMeshAttr = tAttr.create( "inputMesh", "iMesh", MFnData::kMesh );
	tAttr.setHidden( true );
	addAttribute( inputMeshAttr );

	outputMeshAttr = tAttr.create( "outputMesh", "oMesh", MFnData::kMesh );
	tAttr.setHidden( true );
	addAttribute( outputMeshAttr );

	MFnNumericAttribute nAttr;

	inputColorAttr = nAttr.createColor( "color", "ic" );
	addAttribute( inputColorAttr );

	inverseAttr = nAttr.create( "invertDisplace", "id", MFnNumericData::kBoolean, false );
	addAttribute( inverseAttr );

	divisionAttr = nAttr.create( "divisions", "div", MFnNumericData::kInt );
	nAttr.setKeyable( true );
	nAttr.setStorable( true );
	nAttr.setMin( 0 );
	addAttribute( divisionAttr );

	numPassesAttr = nAttr.create( "numberOfPasses", "nPasses", MFnNumericData::kInt, 1 );
	nAttr.setKeyable( true );
	nAttr.setStorable( true );
	nAttr.setMin( 1 );
	addAttribute( numPassesAttr );

	resolutionAttr = nAttr.create( "resolution", "rstn", MFnNumericData::kInt, 10 );
	nAttr.setKeyable( true );
	nAttr.setStorable( true );
	nAttr.setMin( 1 );
	addAttribute( resolutionAttr );
	
	colorThresholdAttr = nAttr.create( "colorThreshold", "cth", MFnNumericData::kFloat, 0.5 );
	nAttr.setKeyable( true );
	nAttr.setStorable( true );
	nAttr.setMin( 0 );
	nAttr.setMax( 1 );
	addAttribute( colorThresholdAttr );

	attributeAffects( inputColorAttr, outputMeshAttr );
	attributeAffects( inverseAttr, outputMeshAttr );
	attributeAffects( divisionAttr, outputMeshAttr );
	attributeAffects( inputMeshAttr, outputMeshAttr );
	attributeAffects( numPassesAttr, outputMeshAttr );
	attributeAffects( colorThresholdAttr, outputMeshAttr );
	attributeAffects( resolutionAttr, outputMeshAttr );

	return MS::kSuccess;
}