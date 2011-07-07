#pragma once
#include <maya/MPxDeformerNode.h>

class displace : public MPxDeformerNode
{
public:
	displace(void);
	virtual ~displace(void);

	static void *creator() { return new displace; }

	static MTypeId id;
	static MString nodeName;
	
	static MObject inputColorAttr;
	static MObject invertDisplaceAttr;
	static MObject displaceScaleAttr;

	virtual MStatus deform( MDataBlock &, MItGeometry&, const MMatrix &, unsigned int );
	static MStatus initialize();
};