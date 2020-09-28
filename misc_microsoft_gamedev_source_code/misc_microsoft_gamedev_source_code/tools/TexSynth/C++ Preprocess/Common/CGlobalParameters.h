/*

  Simple class to handle global parameters, that can be both set by
  program or by a setenv.

  Class is a singleton. 
  Usage:

  GLOBALPARAMETERS.addParameter("filename");
  const char *v=GLOBALPARAMETERS.getValue("filename");
  GLOBALPARAMETERS.setValue("filename","test.txt");

  Sylvain Lefebvre - (c) Microsoft Corp. - 2005-07-07
*/

#ifndef __GLOBALPARAMETERS__
#define __GLOBALPARAMETERS__

#include <string>
#include <map>
#include <hash_map>
#include <iostream>

class GlobalParameters
{
protected:

  typedef std::map<std::string, std::string > t_map;

  t_map   m_Parameters;
  
  GlobalParameters();

public:

  static GlobalParameters *getInstance();

  // Add a parameter. If an environment variable is defined, the
  // paramter will be initialized with its value.
  void addParameter(const char *name);

  // Returns the value of a parameter. NULL is returned if empty parameter.
  const char *getValue(const char *name);
  
  // Set the value of a parameter
  // if 'add' is true, add the parameter if it does not exist
  // return true if success, false otherwise
  bool setValue(const char *name,const char *value,bool add=false);

  // Test if a paramter is defined (define means present + non empty value)
  bool isDefined(const char *name);

  // List defined parameters
  void list(std::ostream& o);

};


#define GLOBALPARAMETERS (*GlobalParameters::getInstance())

#endif
