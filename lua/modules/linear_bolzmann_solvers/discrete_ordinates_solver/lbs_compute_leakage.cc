#include "lbs_do_lua_utils.h"

#include "framework/runtime.h"
#include "framework/logging/log.h"
#include "framework/console/console.h"

#include "framework/mesh/mesh_continuum/mesh_continuum.h"
#include "modules/linear_boltzmann_solvers/b_discrete_ordinates_solver/lbs_discrete_ordinates_solver.h"

namespace opensnlua::lbs
{

RegisterLuaFunctionAsIs(chiLBSComputeLeakage);

int
chiLBSComputeLeakage(lua_State* L)
{
  const std::string fname = "chiLBSComputeLeakage";
  const int num_args = lua_gettop(L);

  if (num_args != 3) LuaPostArgAmountError(fname, 3, num_args);

  LuaCheckNilValue(fname, L, 1);

  // Get pointer to solver
  const int solver_handle = lua_tonumber(L, 1);

  auto& lbs_solver = opensn::Chi::GetStackItem<opensn::lbs::DiscreteOrdinatesSolver>(
    opensn::Chi::object_stack, solver_handle, fname);
  LuaCheckNilValue(fname, L, 2);
  LuaCheckNilValue(fname, L, 3);

  const int groupset_id = lua_tonumber(L, 2);
  const int boundary_id = lua_tonumber(L, 3);

  const auto leakage = lbs_solver.ComputeLeakage(groupset_id, boundary_id);

  // Push up the table
  lua_newtable(L);

  for (int i = 0; i < static_cast<int>(leakage.size()); ++i)
  {
    lua_pushinteger(L, i + 1);
    lua_pushnumber(L, leakage[i]);
    lua_settable(L, -3);
  }

  return 1;
}

RegisterLuaFunction(ComputeLeakage, lbs, ComputeLeakage);

int
ComputeLeakage(lua_State* L)
{
  const auto fname = "lbs.ComputeLeakage";
  const auto num_args = lua_gettop(L);

  // Get the solver
  LuaCheckNilValue(fname, L, 1);
  const auto solver_handle = lua_tointeger(L, 1);
  const auto& solver = opensn::Chi::GetStackItem<opensn::lbs::DiscreteOrdinatesSolver>(
    opensn::Chi::object_stack, solver_handle, fname);

  // Get the boundaries to parse
  std::vector<uint64_t> bndry_ids;
  if (num_args > 1)
  {
    LuaCheckTableValue(fname, L, 2);

    // Define mapping from boundary name to id
    const std::map<std::string, uint64_t> bndry_map = {
      {"xmax", 0}, {"xmin", 1}, {"ymax", 2}, {"ymin", 3}, {"zmax", 4}, {"zmin", 5}};

    // Get the boundaries
    const auto n_bndrys = lua_rawlen(L, 2);
    for (int b = 0; b < n_bndrys; ++b)
    {
      lua_pushinteger(L, b + 1);
      lua_gettable(L, 2);
      bndry_ids.push_back(bndry_map.at(lua_tostring(L, -1)));
      lua_pop(L, 1);
    }
  }
  else
    bndry_ids = solver.Grid().GetDomainUniqueBoundaryIDs();

  // Compute the leakage
  const auto leakage = solver.ComputeLeakage(bndry_ids);

  // Define mapping from boundary ids to names
  const std::map<uint64_t, std::string> bndry_map = {
    {0, "xmax"}, {1, "xmin"}, {2, "ymax"}, {3, "ymin"}, {4, "zmax"}, {5, "zmin"}};

  // Push to lua table
  lua_newtable(L);
  for (const auto& [bid, vals] : leakage)
  {
    lua_pushstring(L, bndry_map.at(bid).c_str());

    lua_newtable(L);
    for (int g = 0; g < solver.NumGroups(); ++g)
    {
      lua_pushinteger(L, g + 1);
      lua_pushnumber(L, vals[g]);
      lua_settable(L, -3);
    }
    lua_settable(L, -3);
  }
  return 1;
}

} // namespace opensnlua::lbs
