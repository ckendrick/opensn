#include "framework/chi_lua.h"

#include <iostream>
#include "framework/mesh/SurfaceMesh/chi_surfacemesh.h"
#include "framework/mesh/MeshHandler/chi_meshhandler.h"

#include "framework/chi_runtime.h"

#include "framework/logging/chi_log.h"
#include "lua_surface_mesh.h"
#include "framework/console/chi_console.h"

RegisterLuaFunctionAsIs(chiSurfaceMeshExportToObj);
RegisterLuaFunctionAsIs(chiSurfaceMeshExportPolyFile);

int
chiSurfaceMeshExportToObj(lua_State* L)
{
  auto& cur_hndlr = chi_mesh::GetCurrentHandler();

  // Get arguments
  int num_args = lua_gettop(L);
  if (num_args != 2) LuaPostArgAmountError("chiSurfaceMeshExportObj", 2, num_args);

  int handle = lua_tonumber(L, 1);

  size_t length = 0;
  const char* temp = lua_tolstring(L, 2, &length);

  auto& surface_mesh =
    Chi::GetStackItem<chi_mesh::SurfaceMesh>(Chi::surface_mesh_stack, handle, __FUNCTION__);

  surface_mesh.ExportToOBJFile(temp);

  return 0;
}

int
chiSurfaceMeshExportPolyFile(lua_State* L)
{
  auto& cur_hndlr = chi_mesh::GetCurrentHandler();

  // Get arguments
  int num_args = lua_gettop(L);
  if (num_args != 2) LuaPostArgAmountError("chiSurfaceMeshExportPolyFile", 2, num_args);

  int handle = lua_tonumber(L, 1);

  size_t length = 0;
  const char* temp = lua_tolstring(L, 2, &length);

  auto& surface_mesh =
    Chi::GetStackItem<chi_mesh::SurfaceMesh>(Chi::surface_mesh_stack, handle, __FUNCTION__);

  surface_mesh.ExportToPolyFile(temp);
  return 0;
}
