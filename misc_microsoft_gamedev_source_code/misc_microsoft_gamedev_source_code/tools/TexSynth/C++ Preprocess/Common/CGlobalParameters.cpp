// ---------------------------------------------------------------------

#include "CGlobalParameters.h"

using namespace std;

// ---------------------------------------------------------------------


GlobalParameters::GlobalParameters()
{

}


// ---------------------------------------------------------------------


GlobalParameters *GlobalParameters::getInstance()
{
  static GlobalParameters* s_GlobalParams=NULL;
  if (s_GlobalParams != NULL)
    return (s_GlobalParams);
  else
    s_GlobalParams = new GlobalParameters();
  return (s_GlobalParams);
}


// ---------------------------------------------------------------------


void GlobalParameters::addParameter(const char *name)
{
  string sname(name);

  t_map::iterator P = m_Parameters.find(sname);
  if (P == m_Parameters.end())
  {
    // new
    m_Parameters[sname] = string("");
    const char *env = getenv(name);
    if (env != NULL)
      m_Parameters[sname]=string(env);
  }
}


// ---------------------------------------------------------------------


const char *GlobalParameters::getValue(const char *name)
{
  string sname(name);

  t_map::iterator P = m_Parameters.find(sname);
  if (P == m_Parameters.end())
    return (NULL);
  else
    return ((*P).second.c_str());
}


// ---------------------------------------------------------------------


bool GlobalParameters::setValue(const char *name,const char *value,bool add)
{
  string sname(name);

  t_map::iterator P = m_Parameters.find(sname);
  if (P == m_Parameters.end())
  {
    if (add)
      addParameter(name);
    else
      return (false);
  }
  m_Parameters[sname]=value;
  return (true);
}


// ---------------------------------------------------------------------


bool GlobalParameters::isDefined(const char *name)
{
  string sname(name);

  t_map::iterator P = m_Parameters.find(sname);
  if (P == m_Parameters.end())
      return (false);
  else
      return ((*P).second.length() > 0);
}


// ---------------------------------------------------------------------


void GlobalParameters::list(ostream& o)
{
  o << endl;
  o << "-== List of global parameters ==- <br>" << endl;
  for (t_map::iterator P=m_Parameters.begin(); P!=m_Parameters.end() ; P++) 
    o << "  * " << (*P).first << "\t = " << (*P).second << "<br>" << endl;
  o << "<br>" << endl;
}


// ---------------------------------------------------------------------
