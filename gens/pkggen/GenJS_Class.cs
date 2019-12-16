using System;
using System.IO;
using System.Collections.Generic;
using System.Reflection;
using System.Text;

public class Info
{
    public string defaultValue;
    public string type;
    public string jsTypeEnum;

    public Info(string defaultValue, string type, string jsTypeEnum)
    {
        this.defaultValue = defaultValue;
        this.type = type;
        this.jsTypeEnum = jsTypeEnum;
    }
}

public static class GenJS_Class
{
    public static Info GetDefaultValue(string type)
    {
        if (type == "PKG::CatchFish::Sits")
        {
            return new Info("0", "number", "DataType.INT32");
        }
        if (type == "xx::List_s<PKG::CatchFish::Sits>")
        {
            return new Info("[]", "[]", "DataType.xx_LIST_SITS");
        }
        if (type.StartsWith("xx::List", StringComparison.Ordinal))
        {
            return new Info("[]", "[]", "DataType.LIST");
        }
        if (type.StartsWith("PKG::", StringComparison.Ordinal))
        {
            //return "null";
            return new Info("null", "any", "DataType.OBJ");
        }
        switch (type)
        {
            case "std::string_s":
                {
                    return new Info("\"\"", "string", "DataType.STRING");
                }
            case "bool":
                {
                    return new Info("false", "bool", "DataType.INT8");
                }
            case "uint8_t":
                {
                    return new Info("0", "number", "DataType.INT8");
                }
            case "int32_t":
                {
                    return new Info("0", "number", "DataType.INT32");
                }
            case "float":
                {
                    return new Info("0.0", "number", "DataType.FLOAT");
                }
            case "double":
                {
                    return new Info("0.0", "number", "DataType.DOUBLE");
                }
            case "int64_t":
                {
                    return new Info("BigInt(0)", "any", "DataType.INT64");
                }
            case "::xx::Pos":
                {
                    // TODO
                    return new Info("null", "any", "DataType.XX_POS");
                }
            case "::xx::Random_s":
                {
                    // TODO
                    return new Info("null", "any", "DataType.XX_RANDOM");
                }
            case "std::weak_ptr<PKG::CatchFish::Fish>":
                {
                    // TODO
                    return new Info("null", "any", "DataType.OBJ");
                }
            default:
                return new Info("null", "any", "DataType.OBJ");
        }
    }

    public static bool Gen(Assembly asm, string outDir, string templateName, string md5, TemplateLibrary.Filter<TemplateLibrary.JsFilter> filter = null, bool generateBak = false)
    {
        var ts = asm._GetTypes();
        var typeIds = new TemplateLibrary.TypeIds(asm);
        //var sb = new StringBuilder();

        var cs = ts._GetClasssStructs();
        //var es = ts._GetEnums();


        var ss = ts._GetStructs();
        if (!generateBak)
        {
            ss._SortByInheritRelation();
        }
        else
        {
            ss._SortByFullName();
        }

        cs = ts._GetClasss();
        if (!generateBak)
        {
            cs._SortByInheritRelation();
        }
        else
        {
            cs._SortByFullName();
        }

        List<string> classes = new List<string>();

        // 预声明
        for (int i = 0; i < cs.Count; ++i)
        {
            var sb = new StringBuilder();
            sb.Append("// @flow\n\n");
            var c = cs[i];
            if (filter != null && !filter.Contains(c)) continue;
            var o = asm.CreateInstance(c.FullName);
            var namespace1 = c.Namespace.Replace(".", "::");


            classes.Add("PKG::" + namespace1 + "::" + c.Name);

            // 定位到基类
            var bt = c.BaseType;
            //var btn = c._HasBaseType() ? bt._GetTypeDecl_Cpp(templateName) : "PkgBase";
            //btn = btn.Replace("::", "_");
            var super = bt._GetTypeDecl_Cpp(templateName);
            var btn = c._HasBaseType() ? super : "PkgBase";
            var idx = btn.LastIndexOf("::", StringComparison.Ordinal);
            if (idx != -1)
            {            
                btn = btn.Substring(idx + 2);
            }

            var deep = namespace1.Replace("::", "$").Split('$').Length + 1;

            sb.Append("const { PkgBase, DataType } = require(\"");
            for (int j = 0; j < deep; ++j)
            {
                sb.Append("../");
            }
            sb.Append("PkgBase\");\n");

            if (c._HasBaseType())
            {
                sb.Append("const " + btn +" = require(\"");
                for (int j = 0; j < deep; ++j)
                {
                    sb.Append("../");
                }
                foreach (var name in super.Replace("::", "$").Split('$'))
                {
                    sb.Append(name + "/");
                }
                sb.Remove(sb.Length - 1, 1);
                sb.Append("\");\n");
            }

            // desc
            // T xxxxxxxxx = defaultValue
            // constexpr T xxxxxxxxx = defaultValue

            var className = "PKG_" + namespace1 + "." + c.Name;
            className = className.Replace('.', '_');

            sb.Append(c._GetDesc()._GetComment_Cpp(0) + "\nclass " + c.Name + " extends " + btn + " {\n");

            // typeId
            sb.Append("    typeId = " + c.Name + ".typeId;");

            // consts( static ) / fields
            var fs = c._GetFieldsConsts();
            foreach (var f in fs)
            {
                var ft = f.FieldType;
                var ftn = ft._GetTypeDecl_Cpp(templateName, "_s");
                var info = GetDefaultValue(ftn);
                sb.Append(f._GetDesc()._GetComment_Cpp(4) + "\n");
                sb.Append("    // " + ftn + "\n");
                if (info.jsTypeEnum == "DataType.INT64")
                {
                    sb.Append("    // $FlowFixMe\n");
                }
                sb.Append("    " + f.Name  + ": " + info.type + " = " + info.defaultValue + ";");
               
            }
            sb.Append("\n\n");

            // push data
            //if (fs.Count > 0)
            {
                sb.Append("    constructor() {\n");
                sb.Append("        super();\n");
                sb.Append("        this.datas.push(\n");
                foreach (var f in fs)
                {
                    var ft = f.FieldType;
                    var ftn = ft._GetTypeDecl_Cpp(templateName, "_s");
                    var info = GetDefaultValue(ftn);
                    sb.Append("            {\n");
                    sb.Append("                type: " + info.jsTypeEnum + ",\n");
                    sb.Append("                key: '" + f.Name + "',\n");
                    sb.Append("            },\n");
                }
                sb.Append("        );\n");
                sb.Append("    }\n\n");

                foreach (var kv in typeIds.types)
                {
                    //if (filter != null && !filter.Contains(kv.Key)) continue;
                    var ct = kv.Key;
                    //if (ct._IsString() || ct._IsBBuffer() || ct._IsExternal() && !ct._GetExternalSerializable()) continue;
                    var typeId = kv.Value;
                    var ctn = ct._GetTypeDecl_Cpp(templateName);

                    System.Console.WriteLine(ctn.Replace("::", "_") + "    " + className.Replace("::", "_"));
                    if (ctn.Replace("::", "_") == className.Replace("::", "_"))
                    {
                        sb.Append("    static typeId = " + typeId + ";\n\n");
                    }
                }
            }
            sb.Append("}\n\n");

            //sb.Append("\n" + c.Name + ".typeId = " + c.Name + ";\n\n");
            sb.Append("module.exports = " + c.Name + ";\n");

            var dirs = (outDir + "/PKG::" + namespace1).Replace("::", "/");
            //dirs = dirs.Replace('.', '/');
            Directory.CreateDirectory(dirs);
            sb._WriteToFile(Path.Combine(dirs, c.Name + ".js"));
        }
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("// @flow\n\n");
            sb.Append("const { MsgDecoder } = require(\"../msg/MsgDecoder\")\n");
            foreach (var classname in classes)
            {
                sb.Append("const " + classname.Replace("::", "_") + " = require(\"./" + classname.Replace("::", "/") + "\");\n");
            }
            sb.Append("\n");
            sb.Append("module.exports = function (md: MsgDecoder) {\n");
            foreach (var classname in classes)
            {
                sb.Append("    md.register(" + classname.Replace("::", "_") + ");\n");
            }
            sb.Append("};\n");


            sb._WriteToFile(Path.Combine(outDir, "RegiserPkgs.js"));
        }

        return true;
    }
}
