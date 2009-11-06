#include "WmBunsenNode.hh"

///////////////////////////////////////////////////////////////////////////////////
// WARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNING
// WARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNING
// The below typeID is NOT A VALID WETA ID
// WARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNING
// WARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNING
///////////////////////////////////////////////////////////////////////////////////

MTypeId WmBunsenNode::typeID( 0x80006 ); 
MString WmBunsenNode::typeName( "wmBunsenNode" );
MObject WmBunsenNode::ca_syncAttrs;
MObject WmBunsenNode::ia_time;
MObject WmBunsenNode::ia_startTime;
MObject WmBunsenNode::ia_rodsNodes;
MObject WmBunsenNode::ia_gravity;
MObject WmBunsenNode::oa_simStepTaken;

WmBunsenNode::WmBunsenNode() : m_initialised( false ), m_beaker( NULL )
{
    m_beaker = new Beaker();
}

WmBunsenNode::~WmBunsenNode()
{
    if ( m_beaker != NULL )
        delete m_beaker;
}

void WmBunsenNode::pullOnAllRodNodes( MDataBlock& i_dataBlock )
{
    MStatus stat;
    
    // Pull all on all input rod nodes, causing them to update the rod data owned by beaker
    // that they each have pointers to.
    MArrayDataHandle inArrayH = i_dataBlock.inputArrayValue( ia_rodsNodes, &stat );
    CHECK_MSTATUS(stat);
    size_t numRodsConnected = inArrayH.elementCount();
  
    for ( unsigned int r=0; r < numRodsConnected; r++ ) 
    {
        inArrayH.jumpToElement( r );
        inArrayH.inputValue( &stat );
        CHECK_MSTATUS( stat );
        
        // and thats it! The rod node will get the signal that it needs to update its output
        // and will directly change the data in m_beaker. It's dumb to pass it along Maya connections
        // to here then to beaker. So we cut out the middle man.
    }
}

void WmBunsenNode::createRodDataFromRodNodes( MDataBlock& i_dataBlock )
{
    MStatus stat;
    
    // Run through each attached rod node and create the associated rod data structure inside 
    // Beaker. This will get called after all the rods have been deleted as Maya has
    // just been moved to start time.
    MArrayDataHandle inArrayH = i_dataBlock.inputArrayValue( ia_rodsNodes, &stat );
    CHECK_MSTATUS(stat);
    size_t numRodsConnected = inArrayH.elementCount();
  
    MPlug rodPlugArray( thisMObject(), ia_rodsNodes );
    CHECK_MSTATUS( stat );
    for ( unsigned int r=0; r < numRodsConnected; r++ ) 
    {
        if ( rodPlugArray.isArray( &stat ) )
        {
            MPlug rodPlug = rodPlugArray.elementByLogicalIndex( r, &stat );
            CHECK_MSTATUS( stat );
            if ( rodPlug.isConnected( &stat ) ) 
            {
                MPlugArray inPlugArr;
                rodPlug.connectedTo( inPlugArr, true, false, &stat );
                CHECK_MSTATUS( stat );
                
                // Since we asked for the destination there can only be one plug in the array
                MPlug rodNodePlug = inPlugArr[0];
                MObject rodNodeObj = rodNodePlug.node( &stat );
                CHECK_MSTATUS( stat );
                MFnDependencyNode rodNodeFn( rodNodeObj );
                WmBunsenRodNode* wmBunsenRodNode = ( WmBunsenRodNode* )rodNodeFn.userNode();
                
                // Since the rod node is purely there to fill in data that comes from its inputs
                // and attributes, we don't let it deal with memory allocation. This node is in 
                // charge of all that.
                m_beaker->resetEverything();
                m_beaker->createSpaceForRods( r, wmBunsenRodNode->numberOfRods() );
                
                wmBunsenRodNode->initialiseRodData( m_beaker->rodData( r ) );
                
                // Now the rod node has used initialised the undeformed postitions for the rods
                // it owns. Since we are resetting the sim we need to actually now create the
                // rods and add them to the world.
                
                m_beaker->createRods( r );
            }
            else
                CHECK_MSTATUS( stat );
        }
    }
}

MStatus WmBunsenNode::compute( const MPlug& i_plug, MDataBlock& i_dataBlock ) 
{
    MStatus stat;
    
   // cerr << "WmBunsenNode::compute plug = " << i_plug.name() << endl;
	
    if ( i_plug == ca_syncAttrs )
    {
        m_previousTime = m_currentTime;
        m_currentTime = i_dataBlock.inputValue( ia_time, &stat ).asTime().value();
		CHECK_MSTATUS( stat );
        
	    m_startTime = i_dataBlock.inputValue( ia_startTime, &stat ).asDouble();
		CHECK_MSTATUS( stat );
        
        const double3 &gravity = i_dataBlock.inputValue( ia_gravity, &stat ).asDouble3();
        CHECK_MSTATUS( stat );
        m_beaker->setGravity( Vec3d( gravity[0], gravity[1], gravity[2] ) );
		
     	if ( m_currentTime == m_startTime )
        {
            createRodDataFromRodNodes( i_dataBlock );
		}
        
        pullOnAllRodNodes( i_dataBlock );
        
		if ( m_currentTime > m_previousTime ) 
        {
          //  if ( m_initialised )
   			{
                // take a step
                m_beaker->takeTimeStep();
            }
    	}

		MDataHandle outputData = i_dataBlock.outputValue ( ca_syncAttrs, &stat );
		if ( !stat )
        {
			stat.perror("WmBunsenNode::compute get ca_syncAttrs");
			return stat;
		}
		
        // We don't even need to put anything in the output handle as nothing uses it.
        // Just tell Maya it's clean so it doesn't repeatedly evaluate it.

		stat = i_dataBlock.setClean( i_plug );
		if ( !stat )
        {
			stat.perror("WmBunsenNode::compute setClean");
			return stat;
		}
	}
    else if ( i_plug == oa_simStepTaken )
    {
        // Get time so Maya knows we care about it.
        i_dataBlock.inputValue( ia_time, &stat ).asTime().value();
        CHECK_MSTATUS(stat);
        
        // Get ca_StepTime so that we know it has moved the sim forward
        i_dataBlock.inputValue( ca_syncAttrs, &stat ).asTime().value();
        CHECK_MSTATUS(stat);
        
        //////////////////////////////////////////////////////////////////
        //
        // We don't actually output any data here. For speed we have the
        // connection that is really a message attribute. It tells the
        // node on the other end that things have changed. It will then
        // get a pointer to this class and grab the data it needs.
        // This could be done with compound attributes or an MPxData class
        // but that is a pain in the ass for such a simple task and we never
        // want to save the data so we don't need to do it.
        //
        //////////////////////////////////////////////////////////////////
        
        stat = i_dataBlock.setClean( i_plug );
        if ( !stat ) 
        {
            stat.perror( "WmBunsenNode::compute oa_simStepTaken setClean()" );
            return stat;
        }
    }
    else
    {
		return MS::kUnknownParameter;
	}

	return MS::kSuccess;
}

void WmBunsenNode::draw( M3dView& i_view, const MDagPath& i_path,
                         M3dView::DisplayStyle i_style,
                         M3dView::DisplayStatus i_status )
{ 
	MStatus stat;
	MObject thisNode = thisMObject();

	MPlug syncPlug( thisNode, ca_syncAttrs );
	double d; 
	stat = syncPlug.getValue( d );
	if ( !stat )
    {
		stat.perror( "WmBunsenNode::draw getting ca_syncAttrs" );
		return;
	}

	i_view.beginGL(); 
    glPushAttrib( GL_CURRENT_BIT | GL_POINT_BIT | GL_LINE_BIT | GL_ENABLE_BIT |  GL_LIGHTING_BIT );
    
    m_beaker->draw();

	// What did this line do? it was here from the devkit example. Is it to with point colouring
	//view.setDrawColor ( WmBunsenNode );

	glPopAttrib();
	i_view.endGL();
}

bool WmBunsenNode::isBounded() const
{ 
	return false;
}

void* WmBunsenNode::creator()
{
	return new WmBunsenNode();
}

MStatus WmBunsenNode::initialize()
{ 
    MStatus stat;

    {
        MFnUnitAttribute	uAttr;
        ia_time = uAttr.create( "time", "t", MTime( 0.0 ), &stat );
        if ( !stat) 
        {
            stat.perror("create ia_time attribute");
            return stat;
        }
        CHECK_MSTATUS( uAttr.setWritable(true) );
        CHECK_MSTATUS( uAttr.setConnectable(true) );
        CHECK_MSTATUS( uAttr.setStorable(false) );
        stat = addAttribute( ia_time );
        if ( !stat ) { stat.perror( "addAttribute ia_time" ); return stat; }
    }
    
    {
        MFnNumericAttribute nAttr;
    	ia_startTime = nAttr.create( "startTime", "stt", MFnNumericData::kDouble, 1.0, &stat );
        if ( !stat ) 
        {
            stat.perror( "create aStartTime attribute");
            return stat;
        }
        nAttr.setWritable( true );
        nAttr.setReadable( false );
        nAttr.setKeyable( true );  
        stat = addAttribute( ia_startTime );
        if ( !stat ) { stat.perror( "addAttribute ia_startTime" ); return stat; }
    }
    
    {
        MFnNumericAttribute nAttr;
        ia_rodsNodes = nAttr.create( "rodsNodes", "rod", MFnNumericData::kBoolean, true, &stat );
        CHECK_MSTATUS (stat );
        CHECK_MSTATUS( nAttr.setWritable( true ) );
        CHECK_MSTATUS( nAttr.setReadable( false ) );
        CHECK_MSTATUS( nAttr.setConnectable( true ) );
        CHECK_MSTATUS( nAttr.setArray( true ) );
        //CHECK_MSTATUS( tAttr.setStorable( false ) );
        stat = addAttribute( ia_rodsNodes );
        if ( !stat ) { stat.perror( "addAttribute ia_rodsNodes" ); return stat;}
    }
    
    {
        MFnNumericAttribute nAttr;
        oa_simStepTaken = nAttr.create( "simStepTaken", "sst", MFnNumericData::kBoolean, true, &stat );
        CHECK_MSTATUS (stat );
        CHECK_MSTATUS( nAttr.setWritable( false ) );
        CHECK_MSTATUS( nAttr.setReadable( true ) );
        CHECK_MSTATUS( nAttr.setConnectable( true ) );
        stat = addAttribute( oa_simStepTaken );
        if ( !stat ) { stat.perror( "addAttribute oa_simulatedRods" ); return stat; }
    }
    
    {
        MFnNumericAttribute nAttr;
        ia_gravity = nAttr.create( "gravity", "gr", MFnNumericData::k3Double, 0, &stat );
        if ( !stat ) {
            stat.perror( "create gravity attribute" );
            return stat;
        }
        nAttr.setWritable( true );
        nAttr.setReadable( false );
        nAttr.setConnectable( true );
        stat = addAttribute( ia_gravity );
        if (!stat) { stat.perror( "addAttribute ia_gravity" ); return stat; }
    }
    
    {
        MFnNumericAttribute	nAttr;
        ca_syncAttrs = nAttr.create( "syncAttrs", "sya", MFnNumericData::kDouble, 1.0, &stat );
        if ( !stat) 
        {
            stat.perror( "create ca_syncAttrs attribute" );
            return stat;
        }
        nAttr.setWritable( false );
        nAttr.setReadable( true );
        nAttr.setConnectable( true );
        nAttr.setKeyable( false );  
        stat = addAttribute( ca_syncAttrs );
        if (!stat) { stat.perror( "addAttribute ca_syncAttrs" ); return stat; }
	}

	stat = attributeAffects( ia_time, ca_syncAttrs );
	if (!stat) { stat.perror( "attributeAffects ia_time->ca_syncAttrs" ); return stat; }
	stat = attributeAffects( ia_startTime, ca_syncAttrs );
	if (!stat) { stat.perror( "attributeAffects ia_startTimer->ca_syncAttrs" ); return stat; }
	stat = attributeAffects( ia_rodsNodes, ca_syncAttrs );
	if (!stat) { stat.perror( "attributeAffects ia_rodsNodes->ca_syncAttrs" ); return stat; }
	stat = attributeAffects( ia_gravity, ca_syncAttrs );
	if (!stat) { stat.perror( "attributeAffects ia_rodsNodes->ca_syncAttrs" ); return stat; }

	stat = attributeAffects( ia_time, oa_simStepTaken );
	if (!stat) { stat.perror( "attributeAffects ia_time->oa_simulatedRods" ); return stat; }
    
	return MS::kSuccess;
}
