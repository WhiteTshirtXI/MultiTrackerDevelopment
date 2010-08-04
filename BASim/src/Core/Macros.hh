/**
 * \file Macros.hh
 *
 * \author miklos@cs.columbia.edu
 * \date 08/31/2009
 */

#ifndef MACROS_HH
#define MACROS_HH

// A collection of macros for adding property-accessing methods to classes that 
//  have property containers. For example, the methods generated by these macros
//  would allow a user to add vertex, edge, or face properties to a class.
// Note: Do not attempt to use a macro twice for a given type of handle.

namespace BASim
{

// BA_ADD_PROPERTY(hndl, container, size)
//   Generates a function "add_property" for adding properties to a property container.
//     hndl:      handle type (e.g. VPropHandle)
//     container: actual property container we want to create an "add_property" function for (e.g. PropertyContainer m_vertexProps)
//     sze:      function call, etc that will give the size of properties
// 
// Note: container.resize(size) resizes the properties in the container, not the container itself.

#define BA_ADD_PROPERTY(hndl, container, sze)                     \
  template <typename T> inline void                               \
  add_property(hndl<T>& ph, const std::string& name, T t = T())   \
  {                                                               \
    if( container.exists<T>(name) ) {                             \
      ph = hndl<T>( container.handle<T>(name) );                  \
    }                                                             \
    else {                                                        \
      ph = hndl<T>( container.add<T>(name) );                     \
      container.resize(sze);                                      \
      container.property(ph).set_default(t);                      \
    }                                                             \
  }

//std::cout << #container << "   " << name << "   " << (container.size()-1) << std::endl;

// BA_ACCESS_PROPERTY(handle, container)
//   Generates a function "property" for accessing properties of a property container.
//     handle:    handle type (e.g. VPropHandle)
//     container: actual property container we want to create a "property" function for (e.g. PropertyContainer m_vertexProps)

#define BA_ACCESS_PROPERTY(handle, container)     \
  template <typename T> inline Property<T>&       \
  property(const handle<T>& ph)                   \
  {                                               \
    return container.property(ph);                \
  }                                               \
                                                  \
  template <typename T> inline const Property<T>& \
  property(const handle<T>& ph) const             \
  {                                               \
    return container.property(ph);                \
  }


// BA_ACCESS_SINGULAR_PROPERTY(handle, container)
//   Generates a function "property" for accessing properties of a property container. Here it is assumed that the property has size 1.
//     handle:    handle type (e.g. VPropHandle)
//     container: actual property container we want to create a "property" function for (e.g. PropertyContainer m_vertexProps)

#define BA_ACCESS_SINGULAR_PROPERTY(handle, container)                \
  template <typename T> inline typename Property<T>::reference        \
  property(const handle<T>& ph)                                       \
  {                                                                   \
    return container.property(ph)[0];                                 \
  }                                                                   \
                                                                      \
  template <typename T> inline typename Property<T>::const_reference  \
  property(const handle<T>& ph) const                                 \
  {                                                                   \
    return container.property(ph)[0];                                 \
  }


// BA_PROPERTY_EXISTS(handle, container)
//   Generates a function "property_exists" for determining if a property is in a property container.
//     handle:    handle type (e.g. VPropHandle)
//     container: actual property container we want to create a "property_exists" function for (e.g. PropertyContainer m_vertexProps)

#define BA_PROPERTY_EXISTS(handle, container)                       \
  template <typename T> inline bool                                 \
  property_exists(const handle<T>&, const std::string& name) const  \
  {                                                                 \
    return container.exists<T>(name);                               \
  }


// BA_PROPERTY_HANDLE(hndl, container, size)
//   Generates a function "property_handle" for retrieving a property from a property container. 
//     hndl:      handle type (e.g. VPropHandle)
//     container: actual property container we want to create a "property_handle" function for (e.g. PropertyContainer m_vertexProps)
//     size:      if property doesn't exist, size each property in the property container is resized to. 

#define BA_PROPERTY_HANDLE(hndl, container, size)                   \
  template <typename T> inline void                                 \
  property_handle(hndl<T>& ph, const std::string& name, T t = T())  \
  {                                                                 \
    if ( container.exists<T>(name) ) {                              \
      ph = hndl<T>( container.handle<T>(name) );                    \
    } else {                                                        \
      ph = hndl<T>( container.add<T>(name) );                       \
      container.resize(size);                                       \
      container.property(ph).set_default(t);                        \
    }                                                               \
  }


// TODO: Kind of annoying having to create an object just to get the proper method called...
// BA_ACCESS_CONTAINER(handle, cont)
//   Generates a function "container" for retrieving a property container from an object. 
//     handle: handle type (e.g. VPropHandle)
//     cont:   actual property container we want to create a "container" function for (e.g. PropertyContainer m_vertexProps)

#define BA_ACCESS_CONTAINER(handle, cont)         \
  template <typename T> PropertyContainer&        \
  container(const handle<T>&)                     \
  {                                               \
    return cont;                                  \
  }                                               \
                                                  \
  template <typename T> const PropertyContainer&  \
  container(const handle<T>&) const               \
  {                                               \
    return cont;                                  \
  }


#define BA_CREATE_PROPERTY(handle, container, size) \
  BA_ADD_PROPERTY(handle, container, size)          \
  BA_ACCESS_PROPERTY(handle, container)             \
  BA_PROPERTY_EXISTS(handle, container)             \
  BA_PROPERTY_HANDLE(handle, container, size)       \
  BA_ACCESS_CONTAINER(handle, container)


#define BA_CREATE_SINGULAR_PROPERTY(handle, container)  \
  BA_ADD_PROPERTY(handle, container, 1)                 \
  BA_ACCESS_SINGULAR_PROPERTY(handle, container)        \
  BA_PROPERTY_EXISTS(handle, container)                 \
  BA_PROPERTY_HANDLE(handle, container, 1)              \
  BA_ACCESS_CONTAINER(handle, container)


#define BA_INHERIT_BASE(base)                   \
  using base::add_property;                     \
  using base::property;                         \
  using base::property_exists;                  \
  using base::property_handle;                  \
  using base::container;

} // namespace BASim

#endif // MACROS_HH
