#include "tesellator.h"
#include "displace.h"
#include "tremble.h"

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

__declspec( dllexport ) MStatus initializePlugin( MObject obj )
{
	MFnPlugin plugFn( obj, "","","" );

	MStatus stat = plugFn.registerNode( tesellator::nodeName, tesellator::id, tesellator::creator, tesellator::initialize );

	if( stat.error() ) {
		MGlobal::displayError( stat.errorString() );
		return MS::kFailure;
	}

	stat = plugFn.registerNode( displace::nodeName, displace::id, displace::creator, displace::initialize, MPxNode::kDeformerNode );

	if( stat.error() ) {
		MGlobal::displayError( stat.errorString() );
		return MS::kFailure;
	}

	stat = plugFn.registerNode( tremble::nodeName, tremble::id, tremble::creator, tremble::initialize );

	if( stat.error() ) {
		MGlobal::displayError( stat.errorString() );
		return MS::kFailure;
	}

	return MS::kSuccess;
}

__declspec( dllexport ) MStatus uninitializePlugin( MObject obj )
{
	MFnPlugin plugFn( obj );

	MStatus stat = plugFn.deregisterNode( tesellator::id );

	if( stat.error() ) {
		MGlobal::displayError( stat.errorString() );
		return MS::kFailure;
	}

	stat = plugFn.deregisterNode( displace::id );

	if( stat.error() ) {
		MGlobal::displayError( stat.errorString() );
		return MS::kFailure;
	}

	stat = plugFn.deregisterNode( tremble::id );

	if( stat.error() ) {
		MGlobal::displayError( stat.errorString() );
		return MS::kFailure;
	}

	return MS::kSuccess;
}