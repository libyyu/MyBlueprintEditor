target("lsocket.static")
	set_kind("static")
	set_basename("lsocket_s")
	set_warnings("all")	

	add_cxflags("-Wno-error=deprecated-declarations", "-fno-strict-aliasing", "-Wno-error=nullability-completeness")
	if is_plat("windows") then
		add_defines("_WINSOCK_DEPRECATED_NO_WARNINGS")
	end

	-- source files
    add_files("*.cpp") 

	add_defines("LUASOCKET_STATIC", {public=true})

    -- platform config
    add_rules("$(plat)")
    add_rules("lua.static")
	--add_rules("target.output.3rd.plugins")
	add_defines("WITH_LSOCKET", "WITH_LSOCKET_STATIC", {public = true})

    -- header files
    add_options("genproj")
    if has_config("genproj") then
        add_headerfiles("*.h")
    end

	if set_group then
        set_group("thirdpart/lua_module")
    else
        set_values("vs.folder", "thirdpart/lua_module")
    end

-- add target
target("lsocket.shared")
    -- set kind
    set_kind("shared")
    set_basename("lsocket")
	-- set warning all and disable error
	set_warnings("all")	

	add_cxflags("-Wno-error=deprecated-declarations", "-fno-strict-aliasing", "-Wno-error=nullability-completeness")
	if is_plat("windows") then
		add_defines("_WINSOCK_DEPRECATED_NO_WARNINGS")
	end

	-- source files
    add_files("*.cpp") 

	add_defines("LUASOCKET_SHARED")

    -- platform config
    add_rules("$(plat)")
    add_rules("lua.shared")
    add_rules("target.output.lua.plugins")
    -- header files
    add_options("genproj")
    if has_config("genproj") then
        add_headerfiles("*.h")
    end
    if set_group then
        set_group("thirdpart/lua_module")
    else
        set_values("vs.folder", "thirdpart/lua_module")
    end
