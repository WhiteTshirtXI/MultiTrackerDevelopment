/**
 * \file ShellTest.cc
 *
 * \author batty@cs.columbia.edu
 * \date April 18, 2011
 */

#include "ShellTest.hh"
#include "BASim/src/Physics/DeformableObjects/DeformableObject.hh"
#include "BASim/src/Physics/DeformableObjects/Shells/ElasticShell.hh"
#include "BASim/src/Physics/DeformableObjects/DefoObjTimeStepper.hh"
#include "BASim/src/Physics/DeformableObjects/Shells/CSTMembraneForce.hh"
#include "BASim/src/Physics/DeformableObjects/Shells/DSBendingForce.hh"
#include "BASim/src/Physics/DeformableObjects/Shells/MNBendingForce.hh"
#include "BASim/src/Physics/DeformableObjects/Shells/ShellGravityForce.hh"
#include "BASim/src/Render/ShellRenderer.hh"
#include "BASim/src/Core/TopologicalObject/TopObjUtil.hh"
#include "BASim/src/Physics/DeformableObjects/Shells/ShellRadialForce.hh"
#include "BASim/src/Physics/DeformableObjects/Shells/ShellSurfaceTensionForce.hh"
#include <fstream>

ShellTest::ShellTest()
: Problem("Shell Test", "A rectangular shell"), 
  shell(NULL), shellObj(NULL), stepper(NULL)
{
  addDynamicsProps();
  
  //Choice of scene
  AddOption("shell-scene", "the shell scene to test", 4);

  //shell options
  AddOption("shell-thickness", "the thickness of the shell", 0.01);
  AddOption("shell-width", "the horizontal side length of the shell", 1.0);
  AddOption("shell-height", "the vertical side length of the shell", 1.0);
  AddOption("x-resolution", "the number of segments along the horizontal edge", 30);
  AddOption("y-resolution", "the number of segments along the vertical edge", 30);
  AddOption("shell-density", "volumetric density of the shell ", 1.0);

  AddOption("surface-tension", "surface tension coefficient of the shell", 1.0);
  
  //thickness-dependent elasticity with proper physical-parameters, only for CST membrane so far.
  AddOption("shell-Poisson", "the Poisson ratio of the shell material", 0.0f);
  AddOption("shell-Youngs", "the Young's modulus of the shell material", 0.0f);
  AddOption("shell-Poisson-damping", "the damping coefficient associated to the shell's Poisson ratio", 0.0f);
  AddOption("shell-Youngs-damping", "the damping coefficient associated with the shell's Young's modulus", 0.0f);

  //DSBend stiffness
  AddOption("shell-bending-stiffness", "Hinge (Discrete shells) bending stiffness of the shell", 0.0);
  AddOption("shell-bending-damping", "Hinge (Discrete shells) bending damping coefficient of the shell ", 0.0);


  //timestepper options
  AddOption("integrator", "type of integrator to use for the shell", "symplectic");
  AddOption("iterations", "maximum number of iterations for the implicit method", (int) 100);
  AddOption("atol", "absolute convergence tolerance", 1e-8);
  AddOption("rtol", "relative convergence tolerance", 1e-8);
  AddOption("stol", "convergence tolerance in terms of the norm of the change in the solution between steps", 1e-8);
  AddOption("inftol", "infinity norm convergence tolerance", 1e-8);
 
  // default to no gravity
  GetVecOpt("gravity") = Vec3d::Zero();
}

ShellTest::~ShellTest()
{
  if (shellObj != NULL) delete shellObj;
  if (shell != NULL) delete shell;
  if (stepper != NULL) delete stepper;
}

typedef void (ShellTest::*sceneFunc)();


sceneFunc scenes[] = {0,
                      &ShellTest::setupScene1, 
                      &ShellTest::setupScene2, 
                      &ShellTest::setupScene3, 
                      &ShellTest::setupScene4, 
                      &ShellTest::setupScene5};

void ShellTest::Setup()
{

  loadDynamicsProps();

  //General shell forces and properties
  Scalar density = GetScalarOpt("shell-density");
  Scalar thickness = GetScalarOpt("shell-thickness");
  
  Vec3d gravity = GetVecOpt("gravity");
  
  Scalar surface_tension = GetScalarOpt("surface-tension");

  Scalar Youngs_modulus = GetScalarOpt("shell-Youngs");
  Scalar Poisson_ratio = GetScalarOpt("shell-Poisson");
  Scalar Youngs_damping = GetScalarOpt("shell-Youngs-damping");
  Scalar Poisson_damping = GetScalarOpt("shell-Poisson-damping");
  
  Scalar DSbendstiffness = GetScalarOpt("shell-bending-stiffness");
  Scalar DSbenddamping = GetScalarOpt("shell-bending-damping");

  std::string integrator = GetStringOpt("integrator");

  //Geometry/scene specific
  int sceneChoice = GetIntOpt("shell-scene");


  

  //Create the base deformable object (mesh)
  shellObj = new DeformableObject();

  (*this.*scenes[sceneChoice])();
  /*
  switch(sceneChoice) {
    case 1: setupScene1();
      break;
    case 2: setupScene2();
      break;
    case 3: setupScene3();
      break;
    case 4: setupScene4();
      break;
    default:
      break;
  }*/
  

  
  bool circular = false;

  
  


  /*
  Vec3d start_vel(0,0,0);
  if(!circular) {
     
     
  }
  else {
    
  }
  */
  
  
  /*
  
  */

  ////choose some initial velocities
  //for(VertexIterator vit = shellObj->vertices_begin(); vit != shellObj->vertices_end(); ++vit) {
  //  Vec3d radius = (positions[*vit] - centre);
  //  radius.normalize();
  //Scalar viscosity = Youngs_damping / 2 / (1 + Poisson_damping);
  //  //velocities[*vit] = pressureStrength/12.0/viscosity/thickness*radius;
  //  //velocities[*vit] = radius;
  //}

 

  //now add forces to the model
  Scalar timestep = getDt(); //Our Rayleigh damping model relies on knowing the timestep (folds it into the damping stiffness, as in Viscous Threads)
 
  //Stretching and bending forces
  if(Youngs_modulus != 0 || Youngs_damping != 0) {
    shell->addForce(new CSTMembraneForce(*shell, "CSTMembrane", Youngs_modulus, Poisson_ratio, Youngs_damping, Poisson_damping, timestep));
    //shell->addForce(new MNBendingForce(*shell, "MNBending", Youngs_modulus, Poisson_ratio, Youngs_damping, Poisson_damping, timestep));
    
  }

  if(DSbendstiffness != 0 || DSbenddamping !=0)
    shell->addForce(new DSBendingForce(*shell, "DSBending", DSbendstiffness, DSbenddamping, timestep));
  
  

  //Gravity force
  shell->addForce(new ShellGravityForce(*shell, "Gravity", gravity));

  //Surface tension force
  if(surface_tension != 0)
    shell->addForce(new ShellSurfaceTensionForce(*shell, "Surface Tension", surface_tension));
  

  //and set its standard properties
  shell->setThickness(thickness);
  shell->setDensity(density);
  
  

 /* std::vector<EdgeHandle> extendEdgeList;
  EdgeIterator eit = shellObj->edges_begin();
  for(;eit != shellObj->edges_end(); ++eit) {
    EdgeHandle eh = *eit;
    VertexHandle vh0 = shellObj->fromVertex(eh);
    VertexHandle vh1 = shellObj->toVertex(eh);
    Vec3d pos0 = shell->getVertexPosition(vh0);
    Vec3d pos1 = shell->getVertexPosition(vh1);
    if(pos0[1] >= -0.01 && pos1[1] >= -0.01) {
      extendEdgeList.push_back(eh);
      std::cout << "Edge: " << eh.idx() << std::endl;
    }
  }
  Vec3d inflow_vel = start_vel;
  shell->setInflowSection(extendEdgeList, inflow_vel);*/


  //for(int q = 0; q < 0; ++q) {
  //  int c = 0; 
  //  std::cout << "Setting up adjusted positions\n";
  //  VertexIterator vit2 = shellObj->vertices_begin();
  //  for(;vit2 != shellObj->vertices_end(); ++vit2) {
  //    //if(c++==0) {
  //    //  continue;
  //    //}
  //    
  //    Vec3d pos = shell->getVertexPosition(*vit2);
  //    pos += Vec3d(0,-dy,0);
  //    shell->setVertexPosition(*vit2, pos);
  //    //std::cout << "After: " << pos << std::endl;
  //  }

  //  shell->extendMesh();
  //}

  //shell->remesh(0.2);
 

  shell->computeMasses();

  //compute the dof indexing for use in the diff_eq solver
  shellObj->computeDofIndexing();

  std::cout << "Set up\n";
  stepper = new DefoObjTimeStepper(*shellObj);
  if(integrator == "symplectic")
    stepper->setDiffEqSolver(DefoObjTimeStepper::SYMPL_EULER);
  else if(integrator == "implicit")
    stepper->setDiffEqSolver(DefoObjTimeStepper::IMPL_EULER);
  else {
    std::cout << "No valid integrator specified. Existing options are 'symplectic' and 'implicit'.\n";
    assert(false);
  }
  stepper->setTimeStep(getDt());
  stepper->setMaxIterations(GetIntOpt("iterations"));
  stepper->set_stol(GetScalarOpt("stol"));
  stepper->set_atol(GetScalarOpt("atol"));
  stepper->set_rtol(GetScalarOpt("rtol"));
  stepper->set_inftol(GetScalarOpt("inftol"));
  
  m_world->addObject(shellObj);
  m_world->addController(stepper);
  RenderBase* shellRender = new ShellRenderer(*shell);
  m_world->addRenderer(shellRender);
  
}

void ShellTest::AtEachTimestep()
{

}

//vertical flat sheet, pinned or flowing at top
void ShellTest::setupScene1() {

  //get params
  Scalar width = GetScalarOpt("shell-width");
  Scalar height = GetScalarOpt("shell-height");
  int xresolution = GetIntOpt("x-resolution");
  int yresolution = GetIntOpt("y-resolution");

  Scalar dx = (Scalar)width / (Scalar)xresolution;
  Scalar dy = (Scalar)height / (Scalar)yresolution;

  //build a rectangular grid of vertices
  std::vector<VertexHandle> vertHandles;
  VertexProperty<Vec3d> undeformed(shellObj);
  VertexProperty<Vec3d> positions(shellObj);
  VertexProperty<Vec3d> velocities(shellObj);

  //edge properties
  EdgeProperty<Scalar> undefAngle(shellObj);
  EdgeProperty<Scalar> edgeAngle(shellObj);
  EdgeProperty<Scalar> edgeVel(shellObj);

  Vec3d start_vel(0,0,0);
  for(int j = 0; j <= yresolution; ++j) {
    for(int i = 0; i <= xresolution; ++i) {
      Vec3d vert(i*dx, j*dy, 0);
     /* if(j < 0.5*yresolution) {
        int k = j;
        int j_mod = (int)(0.5*yresolution);
        vert(1) = j_mod*dx;
        vert(2) = (k-j_mod)*dx;
      }*/
      Vec3d undef = vert;

      VertexHandle h = shellObj->addVertex();

      positions[h] = vert;
      velocities[h] = start_vel;
      undeformed[h] = undef;
      vertHandles.push_back(h);
    }
  }


  //build the faces
  std::vector<Vec3i> tris;
  for(int i = 0; i < xresolution; ++i) {
    for(int j = 0; j < yresolution; ++j) {
      int tl = i + (xresolution+1)*j;
      int tr = i+1 + (xresolution+1)*j;
      int bl = i + (xresolution+1)*(j+1);
      int br = i+1 + (xresolution+1)*(j+1);

      shellObj->addFace(vertHandles[tl], vertHandles[tr], vertHandles[br]);
      shellObj->addFace(vertHandles[tl], vertHandles[br], vertHandles[bl]);
    }
  }

  //create a face property to flag which of the faces are part of the object. (All of them, in this case.)
  FaceProperty<char> shellFaces(shellObj); 
  DeformableObject::face_iter fIt;
  for(fIt = shellObj->faces_begin(); fIt != shellObj->faces_end(); ++fIt)
    shellFaces[*fIt] = true;

  //now create the physical model to hang on the mesh
  shell = new ElasticShell(shellObj, shellFaces);
  shellObj->addModel(shell);

  //positions
  shell->setVertexUndeformed(undeformed);
  shell->setVertexPositions(positions);
  shell->setVertexVelocities(velocities);

  //mid-edge normal variables
  shell->setEdgeUndeformed(undefAngle);
  shell->setEdgeXis(edgeAngle);
  shell->setEdgeVelocities(edgeVel);



  ////find the top left and right corners for adding a constraint
  //VertexIterator vit = shellObj->vertices_begin();
  //Vec3d maxPos(-100,-100,-100), minPos(100,100,100);
  //VertexHandle minH, maxH;
  //for(;vit!= shellObj->vertices_end(); ++vit) {
  //  Vec3d pos = shell->getVertexPosition(*vit);
  //  if(pos[0] <= minPos[0]) {
  //    minH = *vit;
  //    minPos = pos;
  //  }
  //  if(pos[0] >= maxPos[0]) {
  //    maxH = *vit;
  //    maxPos = pos;
  //  }
  //}
  //shell->constrainVertex(minH, minPos);
  //shell->constrainVertex(maxH, maxPos);


  //Find highest vertex
  VertexIterator vit = shellObj->vertices_begin();
  Scalar highest = -10000;
  for(;vit!= shellObj->vertices_end(); ++vit) {
    Vec3d pos = shell->getVertexPosition(*vit);
    if(pos[1] >= highest) {
      highest = pos[1];
    }
  }
  //Pin all verts at or near that height
  for(vit = shellObj->vertices_begin();vit!= shellObj->vertices_end(); ++vit) {
    Vec3d pos = shell->getVertexPosition(*vit);
    if(pos[1] >= highest - 1e-4)
      shell->constrainVertex(*vit, pos);
  }

}

//vertical cylinder, pinned or flowing at top
void ShellTest::setupScene2() {

  //get params
  Scalar width = GetScalarOpt("shell-width");
  Scalar height = GetScalarOpt("shell-height");
  int xresolution = GetIntOpt("x-resolution");
  int yresolution = GetIntOpt("y-resolution");

  Scalar dx = (Scalar)width / (Scalar)xresolution;
  Scalar dy = (Scalar)height / (Scalar)yresolution;

  //build a rectangular grid of vertices
  std::vector<VertexHandle> vertHandles;
  VertexProperty<Vec3d> undeformed(shellObj);
  VertexProperty<Vec3d> positions(shellObj);
  VertexProperty<Vec3d> velocities(shellObj);

  //edge properties
  EdgeProperty<Scalar> undefAngle(shellObj);
  EdgeProperty<Scalar> edgeAngle(shellObj);
  EdgeProperty<Scalar> edgeVel(shellObj);
  
  Vec3d start_vel(0,0,0);
  for(int j = 0; j <= yresolution; ++j) {
    for(int i = 0; i <= xresolution; ++i) {
      Scalar circumference = width;
      Scalar radius = circumference / 2 / pi;
      Scalar angle = ((Scalar)i / (Scalar)(xresolution+1)) * 2 * pi;
      Scalar xpos = radius * cos(angle);
      Scalar zpos = radius * sin(angle);
      Vec3d vert(xpos, -1 + j*dy, zpos);

      Vec3d undef = vert;

      VertexHandle h = shellObj->addVertex();

      positions[h] = vert;
      velocities[h] = start_vel;
      undeformed[h] = undef;
      vertHandles.push_back(h);
    }
  }


  //build the faces
  std::vector<Vec3i> tris;

  for(int i = 0; i < xresolution+1; ++i) {
    for(int j = 0; j < yresolution; ++j) {
      int tl = i + (xresolution+1)*j;
      int tr = (i+1)%(xresolution+1) + (xresolution+1)*j;
      int bl = i + (xresolution+1)*(j+1);
      int br = (i+1)%(xresolution+1) + (xresolution+1)*(j+1);

      shellObj->addFace(vertHandles[tl], vertHandles[tr], vertHandles[br]);
      shellObj->addFace(vertHandles[tl], vertHandles[br], vertHandles[bl]);
    }
    //close the circle
  }

  //create a face property to flag which of the faces are part of the object. (All of them, in this case.)
  FaceProperty<char> shellFaces(shellObj); 
  DeformableObject::face_iter fIt;
  for(fIt = shellObj->faces_begin(); fIt != shellObj->faces_end(); ++fIt)
    shellFaces[*fIt] = true;

  //now create the physical model to hang on the mesh
  shell = new ElasticShell(shellObj, shellFaces);
  shellObj->addModel(shell);

  //positions
  shell->setVertexUndeformed(undeformed);
  shell->setVertexPositions(positions);
  shell->setVertexVelocities(velocities);

  //mid-edge normal variables
  shell->setEdgeUndeformed(undefAngle);
  shell->setEdgeXis(edgeAngle);
  shell->setEdgeVelocities(edgeVel);


  //Find highest vertex
  VertexIterator vit = shellObj->vertices_begin();
  Scalar highest = -10000;
  for(;vit!= shellObj->vertices_end(); ++vit) {
    Vec3d pos = shell->getVertexPosition(*vit);
    if(pos[1] >= highest) {
      highest = pos[1];
    }
  }
  //Pin all verts at or near that height
  for(vit = shellObj->vertices_begin();vit!= shellObj->vertices_end(); ++vit) {
    Vec3d pos = shell->getVertexPosition(*vit);
    if(pos[1] >= highest - 1e-4)
      shell->constrainVertex(*vit, pos);
  }
}

//spherical shell
void ShellTest::setupScene3() {

  int xresolution = GetIntOpt("x-resolution");
  int yresolution = GetIntOpt("y-resolution");

  //build a rectangular grid of vertices
  std::vector<VertexHandle> vertHandles;
  VertexProperty<Vec3d> undeformed(shellObj);
  VertexProperty<Vec3d> positions(shellObj);
  VertexProperty<Vec3d> velocities(shellObj);

  //edge properties
  EdgeProperty<Scalar> undefAngle(shellObj);
  EdgeProperty<Scalar> edgeAngle(shellObj);
  EdgeProperty<Scalar> edgeVel(shellObj);

  //create a sphere
  int layers = yresolution;
  int slices = xresolution;
  Vec3d centre(0,0,0);
  Scalar radius = 1.234;
  Vec3d start_vel(0,0,0);

  std::vector<std::vector<VertexHandle>> vertList;
  //create top pole
  VertexHandle topV = shellObj->addVertex();
  positions[topV] = centre + Vec3d(0,0,radius);
  velocities[topV] = start_vel;
  undeformed[topV] = positions[topV];

  //create bottom pole
  VertexHandle botV = shellObj->addVertex();
  positions[botV] = centre + Vec3d(0,0,-radius);
  velocities[botV] = start_vel;
  undeformed[botV] = positions[botV];

  //fill in the interior
  vertList.resize(layers-1);
  for(int j = 0; j < layers-1; ++j) {
    Scalar heightAngle = -pi/2 + (j+1) * pi/(Scalar)layers;
    for(int i = 0; i < slices; ++i) {
      Scalar rotAngle = 2*pi * (Scalar)i / (Scalar)slices;
      Scalar zVal = radius*sin(heightAngle);
      Scalar newRad = radius*cos(heightAngle);
      Scalar xVal = newRad*cos(rotAngle);
      Scalar yVal = newRad*sin(rotAngle);

      VertexHandle vNew = shellObj->addVertex();
      positions[vNew] = centre + Vec3d(xVal,yVal,zVal);
      velocities[vNew] = start_vel;
      undeformed[vNew] = positions[vNew];
      vertList[j].push_back(vNew);
    }
  }

  //construct faces
  for(int j = 0; j < layers; ++j) {
    for(int i = 0; i < slices; ++i) {
      if(j == 0) {
        shellObj->addFace(botV, vertList[j][i], vertList[j][(i+1)%slices]);
      }
      else if(j == layers-1) {
        shellObj->addFace(topV, vertList[j-1][(i+1)%slices], vertList[j-1][i]);
      }
      else {
        shellObj->addFace(vertList[j-1][i], vertList[j][i], vertList[j][(i+1)%slices]);
        shellObj->addFace(vertList[j-1][i], vertList[j][(i+1)%slices], vertList[j-1][(i+1)%slices]);
      }

    }
  }

  //create a face property to flag which of the faces are part of the object. (All of them, in this case.)
  FaceProperty<char> shellFaces(shellObj); 
  DeformableObject::face_iter fIt;
  for(fIt = shellObj->faces_begin(); fIt != shellObj->faces_end(); ++fIt)
    shellFaces[*fIt] = true;

  //now create the physical model to hang on the mesh
  shell = new ElasticShell(shellObj, shellFaces);
  shellObj->addModel(shell);

  //positions
  shell->setVertexUndeformed(undeformed);
  shell->setVertexPositions(positions);
  shell->setVertexVelocities(velocities);

  //mid-edge normal variables
  shell->setEdgeUndeformed(undefAngle);
  shell->setEdgeXis(edgeAngle);
  shell->setEdgeVelocities(edgeVel);

  //inflate the sphere by some fixed amount
  Scalar inflateDist = 0.1;
  for(VertexIterator vit = shellObj->vertices_begin(); vit != shellObj->vertices_end(); ++vit) {  
    Vec3d oldPos = positions[*vit];
    Vec3d normVec = (oldPos-centre);
    normVec.normalize();
    shell->setVertexPosition(*vit, oldPos + inflateDist*normVec);
  }

  //Add an outward pressure force to inflate the sphere
  //Scalar pressureStrength = 0.1;
  //shell->addForce(new ShellRadialForce(*shell, "Radial", Vec3d(0,0,0), pressureStrength));

}

//simple square with two triangles, one pinned in place, to test bending
void ShellTest::setupScene4() {
  
  //build a rectangular grid of vertices
  std::vector<VertexHandle> vertHandles;
  VertexProperty<Vec3d> undeformed(shellObj);
  VertexProperty<Vec3d> positions(shellObj);
  VertexProperty<Vec3d> velocities(shellObj);

  //edge properties
  EdgeProperty<Scalar> undefAngle(shellObj);
  EdgeProperty<Scalar> edgeAngle(shellObj);
  EdgeProperty<Scalar> edgeVel(shellObj);

  //set up a small chunk of shell for testing bending
  VertexHandle v0 = shellObj->addVertex();
  VertexHandle v1 = shellObj->addVertex();
  VertexHandle v2 = shellObj->addVertex();
  VertexHandle v3 = shellObj->addVertex();
  FaceHandle f0 = shellObj->addFace(v0, v1, v2);
  FaceHandle f1 = shellObj->addFace(v2, v1, v3);

  //set up a square
  positions[v0] = undeformed[v0] = Vec3d(0,0,0);
  positions[v1] = undeformed[v1] = Vec3d(0,0,-1);
  positions[v2] = undeformed[v2] = Vec3d(1,0,0);
  positions[v3] = undeformed[v3] = Vec3d(1,0,-1);
  velocities[v0] = velocities[v1] = velocities[v2] = velocities[v3] = Vec3d(0,0,0);

  //initialize all edges to zero angle for now
  for(EdgeIterator eit = shellObj->edges_begin(); eit!= shellObj->edges_end(); ++eit) {
    EdgeHandle eh = *eit;
    undefAngle[*eit] = edgeAngle[*eit] = edgeVel[*eit] = 0;
  }
  
  //create a face property to flag which of the faces are part of the object. (All of them, in this case.)
  FaceProperty<char> shellFaces(shellObj); 
  DeformableObject::face_iter fIt;
  for(fIt = shellObj->faces_begin(); fIt != shellObj->faces_end(); ++fIt)
    shellFaces[*fIt] = true;

  //now create the physical model to hang on the mesh
  shell = new ElasticShell(shellObj, shellFaces);
  shellObj->addModel(shell);

  //positions
  shell->setVertexUndeformed(undeformed);
  shell->setVertexPositions(positions);
  shell->setVertexVelocities(velocities);

  //mid-edge normal variables
  shell->setEdgeUndeformed(undefAngle);
  shell->setEdgeXis(edgeAngle);
  shell->setEdgeVelocities(edgeVel);


  //CONSTRAINTS

  //Just pin the first triangle right where it is.
  shell->constrainVertex(v0, shell->getVertexPosition(v0));
  shell->constrainVertex(v1, shell->getVertexPosition(v1));
  shell->constrainVertex(v2, shell->getVertexPosition(v2));

}

//a catenary sheet
void ShellTest::setupScene5() {

  //get params
  Scalar width = GetScalarOpt("shell-width");
  Scalar height = GetScalarOpt("shell-height");
  int xresolution = GetIntOpt("x-resolution");
  int yresolution = GetIntOpt("y-resolution");

  Scalar dx = (Scalar)width / (Scalar)xresolution;
  Scalar dy = (Scalar)height / (Scalar)yresolution;

  //build a rectangular grid of vertices
  std::vector<VertexHandle> vertHandles;
  VertexProperty<Vec3d> undeformed(shellObj);
  VertexProperty<Vec3d> positions(shellObj);
  VertexProperty<Vec3d> velocities(shellObj);

  //edge properties
  EdgeProperty<Scalar> undefAngle(shellObj);
  EdgeProperty<Scalar> edgeAngle(shellObj);
  EdgeProperty<Scalar> edgeVel(shellObj);

  Vec3d start_vel(0,0,0);
  for(int j = 0; j <= yresolution; ++j) {
    for(int i = 0; i <= xresolution; ++i) {
      Vec3d vert(i*dx, 0, j*dy);
    /*  if(j < 0.5*yresolution) {
        int k = j;
        int j_mod = (int)(0.5*yresolution);
        vert(1) = j_mod*dx;
        vert(2) = (k-j_mod)*dx;
      }*/
      Vec3d undef = vert;

      VertexHandle h = shellObj->addVertex();

      positions[h] = vert;
      velocities[h] = start_vel;
      undeformed[h] = undef;
      vertHandles.push_back(h);
    }
  }


  //build the faces
  std::vector<Vec3i> tris;
  for(int i = 0; i < xresolution; ++i) {
    for(int j = 0; j < yresolution; ++j) {
      int tl = i + (xresolution+1)*j;
      int tr = i+1 + (xresolution+1)*j;
      int bl = i + (xresolution+1)*(j+1);
      int br = i+1 + (xresolution+1)*(j+1);

      shellObj->addFace(vertHandles[tl], vertHandles[tr], vertHandles[br]);
      shellObj->addFace(vertHandles[tl], vertHandles[br], vertHandles[bl]);
    }
  }

  //create a face property to flag which of the faces are part of the object. (All of them, in this case.)
  FaceProperty<char> shellFaces(shellObj); 
  DeformableObject::face_iter fIt;
  for(fIt = shellObj->faces_begin(); fIt != shellObj->faces_end(); ++fIt)
    shellFaces[*fIt] = true;

  //now create the physical model to hang on the mesh
  shell = new ElasticShell(shellObj, shellFaces);
  shellObj->addModel(shell);

  //positions
  shell->setVertexUndeformed(undeformed);
  shell->setVertexPositions(positions);
  shell->setVertexVelocities(velocities);

  //mid-edge normal variables
  shell->setEdgeUndeformed(undefAngle);
  shell->setEdgeXis(edgeAngle);
  shell->setEdgeVelocities(edgeVel);

  //Find the leftest vertex
  VertexIterator vit = shellObj->vertices_begin();
  Scalar lowest = 10000;
  for(;vit!= shellObj->vertices_end(); ++vit) {
    Vec3d pos = shell->getVertexPosition(*vit);
    if(pos[0] <= lowest) {
      lowest = pos[0];
    }
  }
  //Find the rightest vertex
  vit = shellObj->vertices_begin();
  Scalar highest = -10000;
  for(;vit!= shellObj->vertices_end(); ++vit) {
    Vec3d pos = shell->getVertexPosition(*vit);
    if(pos[0] >= highest) {
      highest = pos[0];
    }
  }

  //Pin all verts at or near that height
  for(vit = shellObj->vertices_begin();vit!= shellObj->vertices_end(); ++vit) {
    Vec3d pos = shell->getVertexPosition(*vit);
    if(pos[0] >= highest - 1e-4) {
      shell->constrainVertex(*vit, pos);
    }
    if(pos[0] <= lowest + 1e-4) {
      shell->constrainVertex(*vit, pos);
    }
  }
 

}






