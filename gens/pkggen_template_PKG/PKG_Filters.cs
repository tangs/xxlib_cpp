#pragma warning disable 0169, 0414
using TemplateLibrary;

[LuaFilter("Generic")]

[JsFilter("Generic")]
[JsFilter("CatchFish")]
[JsFilter("Client_CatchFish")]
[JsFilter("CatchFish_Client")]

[CppFilter("Generic")]
[CppFilter("Client_CatchFish")]
[CppFilter("CatchFish_Client")]
[CppFilter("CatchFish")]
interface IFilters
{
}
