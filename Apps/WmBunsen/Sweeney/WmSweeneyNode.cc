#include "WmSweeneyNode.hh"
#include "WmFigConnectionNode.hh"
#include "../WmBunsenCollisionMeshNode.hh"

#include <maya/MFnMatrixAttribute.h>
#include <maya/MPlugArray.h>

#include <sys/stat.h>

using namespace BASim;


// Required by Maya to identify the node
/* static */ MTypeId WmSweeneyNode::typeID ( 0x001135, 0xF6  );
/* static */ MString WmSweeneyNode::typeName( "wmSweeneyNode" );

// Input attributes
/* static */ MObject WmSweeneyNode::ia_time;
/* static */ MObject WmSweeneyNode::ia_startTime;

// Hair Property Attributes
/* static */ MObject WmSweeneyNode::ia_length;
/* static */ MObject WmSweeneyNode::ia_edgeLength;

// Barbershop specific inputs
/*static*/ MObject WmSweeneyNode::ia_strandVertices;
/*static*/ MObject WmSweeneyNode::ia_verticesPerStrand;

// Sync attributes
/* static */ MObject WmSweeneyNode::ca_timeSync;
/* static */ MObject WmSweeneyNode::ca_rodPropertiesSync;

// Collision meshes
/* static */ MObject WmSweeneyNode::ia_collisionMeshes;
 
WmSweeneyNode::WmSweeneyNode() : m_rodManager( NULL )
{    
}

WmSweeneyNode::~WmSweeneyNode()
{ 
}

MStatus WmSweeneyNode::compute( const MPlug& i_plug, MDataBlock& i_dataBlock )
{
    MStatus status;

    if ( i_plug == ca_rodPropertiesSync )
    {
        // Hair properties
        m_length = i_dataBlock.inputValue( ia_length ).asDouble();
        m_edgeLength = i_dataBlock.inputValue( ia_edgeLength ).asDouble();
        
        MObject strandVerticesObj = i_dataBlock.inputValue( ia_strandVertices ).data();
        MFnVectorArrayData strandVerticesArrayData( strandVerticesObj, &status );
        CHECK_MSTATUS( status );
        
        m_strandVertices = strandVerticesArrayData.array( &status );
        CHECK_MSTATUS( status );
        
        m_numberOfVerticesPerStrand = i_dataBlock.inputValue( ia_verticesPerStrand ).asInt();        
        
        // If we've not yet created all the rods then create them
        if ( m_rodManager == NULL )
        {
            initialiseRodFromBarberShopInput( i_dataBlock );
        }
        else
        {
            // If the rods exist then just update their undeformed configuration but keep running
            // the simulation.
            
            // TODO: Add code to update undeformed configuration 
            // for just now, recreate the rod
            initialiseRodFromBarberShopInput( i_dataBlock );
        }
        
        i_dataBlock.setClean( i_plug );
    }
    else if ( i_plug == ca_timeSync )
    {
        m_currentTime = i_dataBlock.inputValue( ia_time ).asTime().value();
        m_startTime = i_dataBlock.inputValue( ia_startTime ).asDouble();


        if ( m_rodManager == NULL )
        {
        //    MGlobal::displayError( "Please rewind to start time to initialise simulation." );            
        }
        else if ( m_currentTime == m_startTime )
        {            
            initialiseRodFromBarberShopInput( i_dataBlock );
        }
        else
        {
            updateCollisionMeshes( i_dataBlock );
            m_rodManager->takeStep();
        }
        
        i_dataBlock.setClean( i_plug );
    }
    else
    {
        return MS::kUnknownParameter;
    }

    return MS::kSuccess;
}

void WmSweeneyNode::initialiseCollisionMeshes( MDataBlock &i_data )
{
    MStatus status;
    
    MArrayDataHandle inArrayH = i_data.inputArrayValue( ia_collisionMeshes, &status );
    CHECK_MSTATUS( status );
    size_t numMeshesConnected = inArrayH.elementCount();
  
    for ( unsigned int i=0; i < numMeshesConnected; i++ ) 
    {
        // Even if we don't use it, grab the data so Maya knows to evaluate the node    
        inArrayH.jumpToElement(i);
        MDataHandle collisionMeshH = inArrayH.inputValue( &status );
        CHECK_MSTATUS( status );
        
        MPlug plug( thisMObject(), ia_collisionMeshes );
        CHECK_MSTATUS( status );
        if ( plug.isArray( &status ) )
        {
            MPlug indxPlug = plug.elementByLogicalIndex( i, &status );
            CHECK_MSTATUS( status );
            if ( indxPlug.isConnected( &status ) ) 
            {
                MPlugArray inPlugArr;
                indxPlug.connectedTo( inPlugArr, true, false, &status );
                CHECK_MSTATUS( status );
                
                // Since we asked for the destination there can only be one plug in the array
                MPlug meshPlug = inPlugArr[0];
                MObject collisionMeshNodeObj = meshPlug.node( &status );
                CHECK_MSTATUS( status );
                MFnDependencyNode collisionMeshNodeFn( collisionMeshNodeObj );
                WmBunsenCollisionMeshNode* collisionMeshNode = (WmBunsenCollisionMeshNode*)collisionMeshNodeFn.userNode();
         
                TriangleMesh* triangleMesh = NULL;
                WmFigMeshController* figMeshController = NULL;
                
                collisionMeshNode->initialise( NULL, i, &triangleMesh, &figMeshController );
             
                // Now add the mesh to the rod manager
                m_rodManager->addCollisionMesh( triangleMesh, figMeshController );
            }
            else
            {
                CHECK_MSTATUS( status );                
            }
        }
    }
}

void WmSweeneyNode::updateCollisionMeshes( MDataBlock& i_dataBlock )
{
    MStatus status;
    
    MArrayDataHandle inArrayH = i_dataBlock.inputArrayValue( ia_collisionMeshes, &status );
    CHECK_MSTATUS( status );
    size_t numMeshesConnected = inArrayH.elementCount();
  
    for ( unsigned int i=0; i < numMeshesConnected; i++ ) 
    {
        // All we need to do is ask Maya for the data and it will pull the attr,
        // causing a compute in the collision mesh node which will directly
        // update the collision mesh data in the RodManager.
        inArrayH.jumpToElement(i);
        MDataHandle collisionMeshH = inArrayH.inputValue( &status );
        CHECK_MSTATUS( status );
    }
}

void WmSweeneyNode::initialiseRodFromBarberShopInput( MDataBlock& i_dataBlock )
{
    cerr << "initialiseRodFromBarberShopInput() - About to create rods from Barbershop input\n";
    
    // Reset the manager and remove all rods before adding more
    delete m_rodManager;
    
    m_rodManager = new WmSweeneyRodManager();
    
    cerr << "initialiseRodFromBarberShopInput() - Deleted and created a new WmSweeneyRodManager\n";

    if ( m_strandVertices.length() == 0 )
    {
        cerr << "initialiseRodFromBarberShopInput() - no input strands so can't create any rods";
        return;
    }
    
    // First, get all the collision mesh data organised
    initialiseCollisionMeshes( i_dataBlock );
    
    // Create one rod for each barbershop strand. Ignore the strand shape or length but do
    // take its initial direction as a follicle angle
    unsigned int currentVertexIndex = 0;
    unsigned int numberOfStrands = m_strandVertices.length() / m_numberOfVerticesPerStrand;
    
    vector< BASim::Vec3d > vertices;
        
    unsigned int inputStrandNumber = 0;
    while ( inputStrandNumber < numberOfStrands )
    {
        MVector direction = m_strandVertices[ currentVertexIndex + 1 ] 
                            - m_strandVertices[ currentVertexIndex ];
        direction.normalize();
                
        constructRodVertices( vertices, direction, m_strandVertices[ currentVertexIndex ] );
            
        m_rodManager->addRod( vertices, m_currentTime );
        
        currentVertexIndex += m_numberOfVerticesPerStrand;
        inputStrandNumber++;
    }
    
    cerr << "initialiseRodFromBarberShopInput() - About to initialise simulation\n";
    m_rodManager->initialiseSimulation( 1 / 24.0, m_startTime );
    cerr << "initialiseRodFromBarberShopInput() - Simulation initialised\n";
}

void WmSweeneyNode::constructRodVertices( vector< BASim::Vec3d >& o_rodVertices, const MVector& i_direction,
                                  const MVector& i_rootPosition )
{
    cerr << "constructRodVertices(): About to construct rod vertices\n";
    // Construct a list of vertices for a rod with its root at i_rootPosition and going in direction
    // i_direction
    
    o_rodVertices.clear();
    
    MVector edge = i_direction;
    edge.normalize();
    
    MVector currentVertex( i_rootPosition );
    
    double accumulatedLength = 0.0;
    // For now we only do straight hair...
    // 
    // This won't make the rod exactly the requested length but it's close enough for testing
    while ( accumulatedLength < m_length )
    {
        o_rodVertices.push_back( BASim::Vec3d( currentVertex.x, currentVertex.y, currentVertex.z ) );
        
        currentVertex += edge;
        
        accumulatedLength += m_edgeLength;
    }
    
    cerr << "constructRodVertices(): Finished constructing rod vertices\n";    
}

void WmSweeneyNode::draw( M3dView& i_view, const MDagPath& i_path,
                            M3dView::DisplayStyle i_style,
                            M3dView::DisplayStatus i_status )
{
    MStatus status;
    
    // Pull on the sync plugs to cause compute() to be called if any 
    // of the rod properties or time has changed.
    double d;
    MPlug timeSyncPlug( thisMObject(), ca_timeSync );
    timeSyncPlug.getValue( d );
    MPlug propertiesSyncPlug( thisMObject(), ca_rodPropertiesSync );
    propertiesSyncPlug.getValue( d );
    
    i_view.beginGL();
    //glPushAttrib( GL_CURRENT_BIT | GL_POINT_BIT | GL_LINE_BIT );
	glPushAttrib( GL_ALL_ATTRIB_BITS );

	// draw dynamic Hair
    if ( m_rodManager != NULL )
    {
        m_rodManager->drawAllRods();
    }

	glPopAttrib();
	i_view.endGL();
}

MStatus WmSweeneyNode::connectionMade( const  MPlug & plug, const  MPlug & otherPlug, bool asSrc )
{
    MStatus stat;
    MStatus retVal(MS::kUnknownParameter );

    return retVal;
}

MStatus WmSweeneyNode::connectionBroken( const  MPlug & plug, const  MPlug & otherPlug, bool asSrc )
{
    MStatus stat;
    MStatus retVal( MS::kUnknownParameter );

    return retVal;
}

bool WmSweeneyNode::isBounded() const
{
	return false;
}

void* WmSweeneyNode::creator()
{
	return new WmSweeneyNode();
}

/*static */ MStatus WmSweeneyNode::addNumericAttribute( MObject& i_attribute, MString i_longName,
    MString i_shortName, MFnNumericData::Type i_type, double i_defaultValue, bool i_isInput,
    bool i_isArray )
{
    // Creates a numeric attribute with default attributes
    MStatus stat = MS::kSuccess;

    MFnNumericAttribute nAttr;
    i_attribute = nAttr.create( i_longName, i_shortName, i_type, i_defaultValue, &stat );
    if ( !stat )
    {
        cerr << "Failed to create attribute " << i_longName << endl;
        return stat;
    }
    if ( i_isInput )
        nAttr.setWritable( true );
    else
        nAttr.setWritable( false );

    if ( i_isArray )
        nAttr.setArray( true );

    stat = addAttribute( i_attribute );
    if ( !stat ) { stat.perror( "addAttribute " + i_longName ); return stat; }

    return stat;
}

/* static */ MStatus WmSweeneyNode::initialize()
{
    MStatus status;

    addNumericAttribute( ca_timeSync, "timeSync", "syn", MFnNumericData::kBoolean, false, false );
    addNumericAttribute( ca_rodPropertiesSync, "rodPropertiesSync", "rps", MFnNumericData::kBoolean, false, false );

    {
        MFnUnitAttribute uAttr;
        ia_time = uAttr.create( "time", "t", MTime( 0.0 ), &status );
        if ( !status)
        {
            status.perror("create ia_time attribute");
            return status;
        }
        CHECK_MSTATUS( uAttr.setWritable(true) );
        CHECK_MSTATUS( uAttr.setConnectable(true) );
        CHECK_MSTATUS( uAttr.setStorable(false) );
        status = addAttribute( ia_time );
        if ( !status ) { status.perror( "addAttribute ia_time" ); return status; }
    }
    status = attributeAffects( ia_time, ca_timeSync );
	if ( !status ) { status.perror( "attributeAffects ia_time->ca_timeSync" ); return status; }    
    
	addNumericAttribute( ia_startTime, "startTime", "stt", MFnNumericData::kDouble, 1.0, true );
	status = attributeAffects( ia_startTime, ca_timeSync );
	if ( !status ) { status.perror( "attributeAffects ia_startTime->ca_timeSync" ); return status; }

	addNumericAttribute( ia_length, "length", "len", MFnNumericData::kDouble, 10.0, true );
	status = attributeAffects( ia_length, ca_rodPropertiesSync );
	if ( !status ) { status.perror( "attributeAffects ia_length->ca_rodPropertiesSync" ); return status; }

	addNumericAttribute( ia_edgeLength, "edgeLength", "ele", MFnNumericData::kDouble, 1.0, true );
	status = attributeAffects( ia_edgeLength, ca_rodPropertiesSync );
	if ( !status ) { status.perror( "attributeAffects ia_edgeLength->ca_rodPropertiesSync" ); return status; }
    
    {
        MFnTypedAttribute tAttr;  
        ia_strandVertices = tAttr.create( "strandVertices", "sve",
                                          MFnData::kVectorArray, & status );
        CHECK_MSTATUS( status );
        CHECK_MSTATUS( tAttr.setReadable( false ) );
        CHECK_MSTATUS( tAttr.setWritable( true ) );
        CHECK_MSTATUS( tAttr.setStorable( false ) );        
        status = addAttribute( ia_strandVertices );
        CHECK_MSTATUS( status );
    }
    
    {
        MFnNumericAttribute nAttr;
        ia_collisionMeshes = nAttr.create( "collisionMeshes", "com", MFnNumericData::kBoolean, false, &status );
        if (!status) 
        {
            status.perror( "create ia_collisionMeshes attribute" );
            return status;
        }
        nAttr.setWritable( true );
        nAttr.setReadable( false );
        nAttr.setConnectable( true );
        nAttr.setDisconnectBehavior( MFnAttribute::kDelete );
        nAttr.setArray( true );
        status = addAttribute( ia_collisionMeshes );
        if ( !status ) { status.perror( "addAttribute ia_collisionMeshes" ); return status; }
    }
    status = attributeAffects( ia_collisionMeshes, ca_timeSync );
	if ( !status ) { status.perror( "attributeAffects ia_collisionMeshes->ca_timeSync" ); return status; }
    
        
    addNumericAttribute( ia_verticesPerStrand, "verticesPerStrand", "vps", MFnNumericData::kInt, 12, true );
	status = attributeAffects( ia_verticesPerStrand, ca_rodPropertiesSync );
	if ( !status ) { status.perror( "attributeAffects verticesPerStrand->ca_rodPropertiesSync" ); return status; }

    return MS::kSuccess;
}


