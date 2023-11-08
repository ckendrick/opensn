#include "framework/console/chi_console.h"

#include "framework/chi_lua.h"

#include "framework/chi_runtime.h"
#include "framework/utils/chi_timer.h"

namespace chi::lua_utils
{

/**Returns the program time as determined from the home location (involves a
 * collective broadcast).*/
int chiProgramTime(lua_State* L);

RegisterLuaFunctionAsIs(chiProgramTime);

int
chiProgramTime(lua_State* L)
{
  double time;
  if (Chi::mpi.location_id == 0) time = Chi::program_timer.GetTime() / 1000.0;

  MPI_Bcast(&time, 1, MPI_DOUBLE, 0, Chi::mpi.comm);

  lua_pushnumber(L, time);
  return 1;
}

} // namespace chi::lua_utils
