
target("protobuf.static")
   -- make as a static library
   set_kind("static")
   set_basename("protobuf_s")
	-- set warning all and disable error
	set_warnings("all")
	-- set language:  c++11
	set_languages("cxx11")
	-- defines
	add_defines("_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS", {public=true})
	add_includedirs("./", {public=true})

	-- add the common source files
	add_files("google/protobuf/*.cc|testing/*.*|testdata/*.*|compiler/*.*|*test*")
	add_files("google/protobuf/io/*.cc|*test*")
	add_files("google/protobuf/compiler/parser.cc")
	add_files("google/protobuf/compiler/importer.cc")
	if is_plat("windows") then
		add_files("google/protobuf/stubs/*.cc|*test*|atomicops_*")
		--add_files("google/protobuf/stubs/atomicops_internals_x86_msvc.cc")
	elseif is_plat("android") then
		add_files("google/protobuf/stubs/*.cc|*test*|atomicops_*")
		--add_files("google/protobuf/stubs/atomicops_internals_x86_gcc.cc")
	else
		add_files("google/protobuf/stubs/*.cc|*test*")
	end
	
	
	-- platform config
   add_rules("$(plat)")
   add_rules("target.output.3rd.plugins")

   if set_group then
    	set_group("thirdpart/pb")
   else
		set_values("vs.folder", "thirdpart/pb")
	end


target("protobuf.shared")
   -- make as a static library
   set_kind("shared")
   set_basename("protobuf")
	-- set warning all and disable error
	set_warnings("all")
	-- set language:  c++11
	set_languages("cxx11")
	-- defines
	add_defines("LIBPROTOBUF_EXPORTS")
	add_defines("_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS", {public=true})
	add_defines("PROTOBUF_USE_DLLS", {public=true})
	add_includedirs("./", {public=true})

	-- add the common source files
	add_files("google/protobuf/*.cc|testing/*.*|testdata/*.*|compiler/*.*|*test*")
	add_files("google/protobuf/io/*.cc|*test*")
	add_files("google/protobuf/compiler/parser.cc")
	add_files("google/protobuf/compiler/importer.cc")
	if is_plat("windows") then
		add_files("google/protobuf/stubs/*.cc|*test*|atomicops_*")
		--add_files("google/protobuf/stubs/atomicops_internals_x86_msvc.cc")
	elseif is_plat("android") then
		add_files("google/protobuf/stubs/*.cc|*test*|atomicops_*")
		--add_files("google/protobuf/stubs/atomicops_internals_x86_gcc.cc")
	else
		add_files("google/protobuf/stubs/*.cc|*test*")
	end

	-- platform config
   add_rules("$(plat)")
   add_rules("target.output.3rd.plugins")

   if set_group then
    	set_group("thirdpart/pb")
   else
		set_values("vs.folder", "thirdpart/pb")
	end

 -- target("protoc.binary")
 -- 	-- make as a static library
 --    set_kind("binary")
 --    set_basename("protoc")
 --    -- set warning all and disable error
-- 	set_warnings("all")
-- 	-- set language:  c++11
-- 	set_languages("cxx11")

 --    add_deps("protobuf.static")

 --    add_files("google/protobuf/compiler/*.cc|parser.cc|importer.cc|*test*|*unittest*|mock_code_generator.cc")
 --    add_files("google/protobuf/compiler/cpp/*.cc|*test*|*unittest*")
 --    add_files("google/protobuf/compiler/csharp/*.cc|*test*|*unittest*")
 --    add_files("google/protobuf/compiler/java/*.cc|*test*|*unittest*")
 --    add_files("google/protobuf/compiler/js/*.cc|*test*|*unittest*")
 --    add_files("google/protobuf/compiler/objectivec/*.cc|*test*|*unittest*")
 --    add_files("google/protobuf/compiler/php/*.cc|*test*|*unittest*")
 --    add_files("google/protobuf/compiler/python/*.cc|*test*|*unittest*")
 --    add_files("google/protobuf/compiler/ruby/*.cc|*test*|*unittest*")
 --    add_includedirs("../", "../protobuf")
 --    -- platform config
 --   add_rules("$(plat)")
 --   if set_group then
 --    	set_group("thirdpart/pb")
 --   else
-- 		set_values("vs.folder", "thirdpart/pb")
-- 	end

    
