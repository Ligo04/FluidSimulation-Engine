set_xmakever("2.7.9")
set_version("1.0.0")
add_rules("mode.release", "mode.debug")

rule("dx_lib")
    on_load(function (target)
        target:add("syslinks","d3d11","dxgi","dxguid","D3DCompiler","winmm","kernel32","user32","gdi32","winspool","shell32","ole32","oleaut32","uuid","comdlg32","advapi32")
        bin_path = path.join(os.projectdir(),"bin")
    end)
rule_end()

rule("imguiini")
    after_build(function ()
        print("after_build")
        imguiini_file=path.join(os.projectdir(),"imgui.ini")
        bin_path = path.join(os.projectdir(),"bin")
        if not os.isdir(bin_path) then
            os.mkdir(bin_path)
        end
        if os.isfile(imguiini_file) then
            os.mv(imguiini_file,bin_path)
        end
    end)
rule_end()

if is_arch("x64", "x86_64", "arm64") then
	-- disable ccache in-case error
	set_policy("build.ccache", true)
	includes("xmake_func.lua")
    includes("thirdparty")
    add_cxxflags("/utf-8")

    target("FluidSimulation-Engine")
        add_rules("imguiini")
        add_rules("hlsl_shader_complier")
        -- set bin dir
        set_targetdir("bin")
        _config_project({
            project_kind = "binary",
            enable_exception = true,
            --batch_size=8
        })
        add_deps("imgui")
        add_rules("dx_lib")
        add_includedirs("include")
        add_headerfiles("include/**.h")
        add_files("src/**.cpp")
        --shader
        add_headerfiles("shaders/**.hlsl|shaders/**.hlsli")
        add_files("shaders/**.hlsl|shaders/**.hlsli")
    target_end()
end 
