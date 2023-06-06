rule("hlsl_shader_complier")
    set_extensions(".hlsl",".hlsli")
    on_buildcmd_files(function (target,batchcmds,sourcebatch, opt)
		import("lib.detect.find_program")
		local msvc = target:toolchain("msvc")
		local runenvs = msvc:runenvs()
		local fxcopt = {}
		fxcopt.envs = runenvs
		fxcopt.force = true
		fxcopt.check   = opt.check or function () end
		fxc = find_program(opt.program or "fxc", fxcopt)
		
		hlsl_generated_dir=path.join(os.projectdir(),"\\shaders\\generated")
		if not os.isdir(hlsl_generated_dir) then
			os.mkdir(hlsl_generated_dir)
		end
		
        for _,sourcefile_hlsl in ipairs(sourcebatch.sourcefiles) do 
            local ext = path.extension(sourcefile_hlsl)
            if ext==".hlsl" then 
                local hlsl_basename = path.basename(sourcefile_hlsl)
                local hlsl_output_path = path.join(hlsl_generated_dir,hlsl_basename..".cso")
                local hlsl_shader_entrypoint = string.sub(hlsl_basename,-2)
				batchcmds:show_progress(opt.progress, "${color.build.object}compiling.hlsl %s", sourcefile_hlsl)
                if is_mode("debug") then  
					batchcmds:vrunv(fxc,{sourcefile_hlsl,"/Zi","/Od","/E",hlsl_shader_entrypoint,"/Fo",hlsl_output_path,"/T",string.lower(hlsl_shader_entrypoint).."_5_0","/nologo"})
                else 
					batchcmds:vrunv(fxc,{sourcefile_hlsl,"/E",hlsl_shader_entrypoint,"/Fo",hlsl_output_path,"/T",string.lower(hlsl_shader_entrypoint).."_5_0","/nologo"})
                end 
            end
        end 
    end)
rule_end()


rule("engine_basic_settings")
on_config(function(target)
	local _, cc = target:tool("cxx")
	if is_plat("linux") then
		-- Linux should use -stdlib=libc++
		-- https://github.com/LuisaGroup/LuisaCompute/issues/58
		if (cc == "clang" or cc == "clangxx" or cc == "gcc" or cc == "gxx") then
			target:add("cxflags", "-stdlib=libc++", {
				force = true
			})
		end
	end
end)
on_load(function(target)
	local _get_or = function(name, default_value)
		local v = target:values(name)
		if v == nil then
			return default_value
		end
		return v
	end
	local project_kind = _get_or("project_kind", "phony")
	target:set("kind", project_kind)
	local c_standard = target:values("c_standard")
	local cxx_standard = target:values("cxx_standard")
	if type(c_standard) == "string" and type(cxx_standard) == "string" then
		target:set("languages", c_standard, cxx_standard)
	else
		target:set("languages", "c11", "cxx17")
	end

	local enable_exception = _get_or("enable_exception", nil)
	if enable_exception then
		target:set("exceptions", "cxx")
	else
		target:set("exceptions", "no-cxx")
	end
	if is_mode("debug") then
		target:add("defines","_DEBUG","DEBUGE")
		target:set("runtimes", "MDd")
		target:set("optimize", "none")
		target:set("warnings", "none")
		target:add("cxflags", "/GS", "/W3","/Gd","/Ob1","/Od",{
			tools = {"msvc"}
		})
		target:add("cxflags", "/Zc:preprocessor", {
			tools = {"msvc"}
		});
	else
		target:set("defines","NODEBUG")
		target:set("runtimes", "MD")
		target:set("optimize", "fast")
		target:set("warnings", "none")
		target:add("cxflags", "/Oy", "/GS-", "/Gd", "/Oi", "/Ot", "/GT", "/Ob2", {
			tools = {"msvc"}
		})
		target:add("cxflags", "/Zc:preprocessor", {
			tools = {"msvc"}
		})
	end
	target:add("defines","UNICODE","_UNICODE","_WIN32","_WINDOWS")
end)
rule_end()
-- In-case of submod, when there is override rules, do not overload
if _config_rules == nil then
	_config_rules = {"engine_basic_settings"}
end
if _disable_unity_build == nil then
	_disable_unity_build = not get_config("enable_unity_build")
end

if _configs == nil then
	_configs = {}
end
function _config_project(config)
	if type(_configs) == "table" then
		for k, v in pairs(_configs) do
			set_values(k, v)
		end
	end
	if type(_config_rules) == "table" then
		add_rules(_config_rules)
	end
	if type(config) == "table" then
		for k, v in pairs(config) do
			set_values(k, v)
		end
	end
	local batch_size = config["batch_size"]
	if type(batch_size) == "number" and batch_size > 1 and (not _disable_unity_build) then
		add_rules("c.unity_build", {
			batchsize = batch_size
		})
		add_rules("c++.unity_build", {
			batchsize = batch_size
		})
	end
end