#pragma once

#include <maya/MPxNode.h>
#include <maya/MImage.h>

class tesellator : public MPxNode
{
public:
	tesellator(void);
	virtual ~tesellator(void);

	static void *creator() { return new tesellator; }
	virtual MStatus compute( const MPlug &, MDataBlock & );

	void subdivide( MObject &, MDataBlock &, MImage * );
	void subdivide( MObject &, MDataBlock &, MString );

	static MStatus initialize();

	static MObject colorThresholdAttr;
	static MObject divisionAttr;
	static MObject numPassesAttr;
	static MObject inputMeshAttr;

	static MObject resolutionAttr;

	static MObject inverseAttr;

	static MObject inputColorAttr;

	static MObject outputMeshAttr;

	static MTypeId id;

	static MString nodeName;
};