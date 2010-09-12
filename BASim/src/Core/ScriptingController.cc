/**
 * \file ScriptingController.cc
 *
 * \author smith@cs.columbia.edu
 * \date 07/21/2010
 */

#include "BASim/src/Core/ScriptingController.hh"

namespace BASim
{

ScriptingController::ScriptingController( double time, double dt )
: m_time(time)
, m_dt(dt)
{}

void ScriptingController::setTime( double time )
{
  m_time = time;
}

void ScriptingController::setDt( double dt )
{
  m_dt = dt;
}
  
double ScriptingController::getTime()
{
  return m_time;
}

double ScriptingController::getDt()
{
  return m_dt;
}


} // namespace BASim