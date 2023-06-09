set_group("thirdparty")
target("imgui")
_config_project({
    project_kind = "static",
    enable_exception = true,
    batch_size=8
})
add_includedirs("imgui", {
    public = true
})
if is_plat("windows") then
    add_defines("IMGUI_API=__declspec(dllexport)")
end
add_headerfiles("imgui/**.h")
add_files("imgui/**.cpp")
target_end()