#pragma once

#include <maya/MPxNode.h>

class tremble : public MPxNode
{
public:
	tremble(void);
	~tremble(void);

	static void *creator() { return new tremble; }

	virtual MStatus compute( const MPlug &plug, MDataBlock &block );
	static MStatus initialize();

	void subdivide( MObject &inputMesh, MDataBlock &block );

	static MTypeId id;
	static MString nodeName;

	static MObject inputMeshAttr;
	static MObject outputMeshAttr;
	static MObject bufferDistanceAttr;
	static MObject divisionsAttr;
	static MObject numPassesAttr;

	static MObject collisionObjectsAttr;
};
