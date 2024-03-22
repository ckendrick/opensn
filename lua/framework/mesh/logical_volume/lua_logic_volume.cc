#include "lua_logic_volume.h"
#include "framework/mesh/logical_volume/logical_volume.h"
#include "framework/runtime.h"
#include "framework/console/console.h"

using namespace opensn;

namespace opensnlua
{

RegisterLuaFunctionNamespace(LogVolPointSense, logvol, PointSense);

int
LogVolPointSense(lua_State* L)
{
  const std::string fname = "logvol.PointSense";
  LuaCheckArgs<int, Vector3>(L, fname);

  const auto lv_handle = LuaArg<int>(L, 1);
  auto point = LuaArg<Vector3>(L, 2);

  const auto& lv = opensn::GetStackItem<LogicalVolume>(opensn::object_stack, lv_handle, fname);

  if (lv.Inside(point))
    LuaPush(L, true);
  else
    LuaPush(L, false);

  return 1;
}

} // namespace opensnlua
