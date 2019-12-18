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
    public bool isObj;

    public Info(string defaultValue, string type, string jsTypeEnum, bool isObj)
    {
        this.defaultValue = defaultValue;
        this.type = type;
        this.jsTypeEnum = jsTypeEnum;
        this.isObj = isObj;
    }
}

public static class GenJS_Class
{
    public static string GetClassName(string type)
    {
        var str = type;
        int startIdx;
        int endIdx;
        while ((startIdx = str.IndexOf("<", StringComparison.Ordinal)) != -1)
        {
            endIdx = str.LastIndexOf(">", StringComparison.Ordinal);
            str = str.Substring(startIdx + 1, endIdx - startIdx - 1);
        }
        endIdx = str.LastIndexOf("_s", StringComparison.Ordinal);
        if (endIdx != -1)
        {
            str = str.Substring(0, endIdx);
        }
        return str.Replace("::", "__");
    }

    public static string RepeatString(string str, int times)
    {
        var sb = new StringBuilder();
        for (int i = 0; i < times; ++i)
        {
            sb.Append(str);
        }
        return sb.ToString();
    }

    public static string NSpace(int n)
    {
        return RepeatString(" ", n);
    }

    public static string ConverJsClassName(string path)
    {
        path = path.Replace("__", "/").Replace("::", "/");
        var jsClassName = "";
        var idx1 = path.LastIndexOf("/", StringComparison.Ordinal);
        if (idx1 == -1)
        {
            jsClassName = path;
            path = "";
        }
        else
        {
            jsClassName = path.Substring(idx1 + 1);
            path = path.Substring(0, idx1 + 1);
        }

        var sb = new StringBuilder(path);
        var len = jsClassName.Length;
        for (int i = 0; i < len; ++i)
        {
            char ch = jsClassName[i];
            if (ch == '_')
            {
                continue;
            }
            else if (Char.IsUpper(ch))
            {
                if (i != 0)
                {
                    sb.Append("-");
                }
                sb.Append(Char.ToLower(ch));
            }
            else
            {
                sb.Append(ch);
            }
        }
        return sb.ToString().ToLower();
    }

    public static Info GetDefaultValue(string type)
    {
        // 处理特殊情况
        if (type == "PKG::CatchFish::Sits")
        {
            return new Info("0", "number", "DataType.INT32", false);
        }
        if (type == "xx::List_s<PKG::CatchFish::Sits>")
        {
            return new Info("[]", "number[]", "DataType.xx_LIST_SITS", false);
        }
        if (type == "xx::List_s<int32_t>")
        {
            return new Info("[]", "number[]", "DataType.LIST_INT32", false);
        }
        if (type == "xx::List_s<::xx::Pos>" || type == "xx::List_s<xx::List_s<::xx::Pos>>")
        {
            return new Info("[]", "[]", "DataType.LIST", false);
        }
        if (type == "xx::List_s<PKG::CatchFish::WayPoint>")
        {
            return new Info("[]", "[]", "DataType.LIST_WAY_POINT", false);
        }

        // 处理通用情况
        if (type.StartsWith("xx::List", StringComparison.Ordinal))
        {
            var className = GetClassName(type);
            return new Info("[]", className + "[]", "DataType.LIST", true);
        }
        if (type.StartsWith("PKG::", StringComparison.Ordinal))
        {
            var className = GetClassName(type);
            return new Info("", className, "DataType.OBJ", true);
        }
        if (type.StartsWith("std::weak_ptr", StringComparison.Ordinal))
        {
            var className = GetClassName(type);
            return new Info("", className, "DataType.OBJ", true);
        }
        switch (type)
        {
            case "std::string_s":
                {
                    return new Info("\"\"", "string", "DataType.STRING", false);
                }
            case "bool":
                {
                    return new Info("false", "bool", "DataType.INT8", false);
                }
            case "uint8_t":
                {
                    return new Info("0", "number", "DataType.INT8", false);
                }
            case "int32_t":
                {
                    return new Info("0", "number", "DataType.INT32", false);
                }
            case "float":
                {
                    return new Info("0.0", "number", "DataType.FLOAT", false);
                }
            case "double":
                {
                    return new Info("0.0", "number", "DataType.DOUBLE", false);
                }
            case "int64_t":
                {
                    return new Info("BigInt(0)", "any", "DataType.INT64", false);
                }
            case "::xx::Pos":
                {
                    // TODO
                    return new Info("null", "any", "DataType.XX_POS", false);
                }
            case "::xx::Random_s":
                {
                    // TODO
                    return new Info("null", "any", "DataType.XX_RANDOM", false);
                }
            //case "std::weak_ptr<PKG::CatchFish::Fish>":
            //    {
            //        // TODO
            //        return new Info("null", "any", "DataType.OBJ", true);
            //    }
            default:
                return new Info("", "any", "DataType.OBJ", true);
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
            sb.Append("pkg-base\");\n");

            var fs = c._GetFieldsConsts();
            var includeSet = new HashSet<string>();
            foreach (var f in fs)
            {
                var sb1 = new StringBuilder();
                var ft = f.FieldType;
                var ftn = ft._GetTypeDecl_Cpp(templateName, "_s");
                if (!GetDefaultValue(ftn).isObj) continue;
                var className1 = GetClassName(ftn);
                //var jsCalssName = ConverJsClassName(className1);
                var path = className1.Replace("__", "/");
                var jsClassName = ConverJsClassName(path);


                sb1.Append("const " + className1 + " = require(\"");
                for (int j = 0; j < deep; ++j)
                {
                    sb1.Append("../");
                }

                sb1.Append(jsClassName + "\");\n");
                includeSet.Add(sb1.ToString());
            }

            // require super class
            if (c._HasBaseType())
            {
                sb.Append("const " + btn +" = require(\"");
                for (int j = 0; j < deep; ++j)
                {
                    sb.Append("../");
                }
                var name1 = ConverJsClassName(super);
                sb.Append(name1);
                //foreach (var name in super.Replace("::", "$").Split('$'))
                //{
                //    sb.Append(name + "/");
                //}
                //sb.Remove(sb.Length - 1, 1);
                sb.Append("\");\n");
            }

            sb.Append("\n");
            foreach (var str in includeSet)
            {
                sb.Append(str);
            }

            // desc
            // T xxxxxxxxx = defaultValue
            // constexpr T xxxxxxxxx = defaultValue

            var className = "PKG_" + namespace1 + "." + c.Name;
            className = className.Replace('.', '_');

            sb.Append(c._GetDesc()._GetComment_Cpp(0) + "\nclass " + c.Name + " extends " + btn + " {\n");

            // typeId
            sb.Append("    typeId = " + c.Name + ".typeId;\n");

            //sb.Append("    // $FlowFixMe\n");
            //sb.Append("    props: {}  = {");
            // consts( static ) / fields
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
                //sb.Append("        " + f.Name + ": " + info.defaultValue + ",");
                sb.Append("    " + f.Name + ": " + info.type);
                if (info.defaultValue.Length > 0)
                {
                    sb.Append(" = " + info.defaultValue);
                }
                sb.Append(";");

            }
            //sb.Append("\n    };");
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

            var dirs = (outDir + "/PKG::" + namespace1).Replace("::", "/").ToLower();
            //dirs = dirs.Replace('.', '/');
            var jsFilename = ConverJsClassName(c.Name);
            Directory.CreateDirectory(dirs);
            sb._WriteToFile(Path.Combine(dirs, jsFilename + ".js"));
        }
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("// @flow\n\n");
            sb.Append("const { MsgDecoder } = require(\"../msg/msg-decoder\")\n");
            foreach (var classname in classes)
            {
                var name1 = ConverJsClassName(classname);
                sb.Append("const " + classname.Replace("::", "_") + " = require(\"./" + name1 + "\")\n");
            }
            sb.Append("\n");
            sb.Append("module.exports = function (md: MsgDecoder) {\n");
            foreach (var classname in classes)
            {
                sb.Append("    md.register(" + classname.Replace("::", "_") + ");\n");
            }
            sb.Append("};\n");


            sb._WriteToFile(Path.Combine(outDir, "regiser-pkgs.js"));
        }

        return true;
    }
}
