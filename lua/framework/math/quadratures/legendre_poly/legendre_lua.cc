#include "framework/lua.h"
#include "framework/math/quadratures/angular/legendre_poly/legendrepoly.h"
#include "framework/console/console.h"
#include "legendre_lua.h"

using namespace opensn;

namespace opensnlua
{

RegisterLuaFunctionNamespace(Legendre, aquad, Legendre);
RegisterLuaFunctionNamespace(LegendreDerivative, aquad, LegendreDerivative);
RegisterLuaFunctionNamespace(Ylm, aquad, Ylm);

int
Legendre(lua_State* L)
{
  LuaCheckArgs<int, double>(L, "aquad.Legendre");
  auto N = LuaArg<int>(L, 1);
  auto x = LuaArg<double>(L, 2);
  double retval = opensn::Legendre(N, x);
  LuaPush(L, retval);
  return 1;
}

int
LegendreDerivative(lua_State* L)
{
  LuaCheckArgs<int, double>(L, "aquad.LegendreDerivative");
  auto N = LuaArg<int>(L, 1);
  auto x = LuaArg<double>(L, 2);
  double retval = dLegendredx(N, x);
  LuaPush(L, retval);
  return 1;
}

int
Ylm(lua_State* L)
{
  LuaCheckArgs<int, int, double, double>(L, "aquad.Ylm");
  auto ell = LuaArg<int>(L, 1);
  auto m = LuaArg<int>(L, 2);
  auto theta = LuaArg<double>(L, 3);
  auto varphi = LuaArg<double>(L, 4);
  double retval = opensn::Ylm(ell, m, varphi, theta);
  LuaPush(L, retval);
  return 1;
}

} // namespace opensnlua
