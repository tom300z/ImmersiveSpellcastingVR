-- set minimum xmake version
set_xmakever("2.8.2")

-- includes
includes("lib/commonlibsse-ng")

-- set project
set_project("ImmersiveSpellcastingVR")
set_version("0.0.0")
set_license("MIT")

-- set defaults
set_languages("c++23")
set_warnings("allextra")

-- set policies
set_policy("package.requires_lock", true)

-- add rules
add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

-- custom stuff
set_config("skyrim_se", false)
set_config("skyrim_ae", false)
set_config("skyrim_vr", true)

-- Papyrus rule
rule("papyrus")
    set_extensions(".psc")
    on_build_files(function (target, sourcebatch, opt)
        import("core.project.depend")
        print(target:installdir())
        depend.on_changed(function ()
            os.exec("scripts\\compile_papyrus_mo2.cmd " .. target:installdir())
            print("Compiled papyrus")
        end, {files = sourcebatch.sourcefiles})
    end)

-- hkx animation rule
rule("hkx-anim")
    set_extensions(".xml")
    on_build_file(function (target, sourcefile, opt)
        if not sourcefile:match(".-[/\\]animations[/\\].+%.xml$") then
            -- Not an animation, don't process
            return
        end
        import("core.project.depend")

        depend.on_changed(function ()
            -- process the file
            local animation_subfolder = path.relative(path.directory(sourcefile), path.join(os.projectdir(), "mod_data_source"))
            local outdir = path.join(target:installdir(), animation_subfolder)
            os.mkdir(outdir)
            os.exec("hkxconv.exe convert -v hkx " .. sourcefile .. " " .. outdir)
        end, {files = sourcefile})
    end)

-- targets
target("ImmersiveSpellcastingVR")
    -- add dependencies to target
    add_deps("commonlibsse-ng")

    -- add commonlibsse-ng plugin
    add_rules("commonlibsse-ng.plugin", {
        name = "ImmersiveSpellcastingVR",
        author = "tom300z",
        description = "SKSEVR Plugin that makes VR Spellcasting more immersive."
    })

    -- add src files
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")

    add_linkdirs("lib/commonlibsse-ng/extern/openvr/lib/win64")
    add_links("openvr_api")

    -- papyrus
    add_rules("papyrus")
    add_files("mod_data_source/scripts/**.psc")

    -- animations
    add_rules("hkx-anim")
    add_files("mod_data_source/**/animations/**.xml")

    -- custom
    add_ldflags("/INCREMENTAL:NO", {force = true})  -- Ensure the new dll is always copied so it matches the symbols
    --add_installfiles("$(buildir)/$(plat)/$(arch)/$(mode)/ImmersiveSpellcastingVR.pdb")
    after_build(function(target)
        if is_mode("debug") then
            os.exec("scripts\\kill_skyrimvr.bat")
            os.exec("scripts\\start_skyrimvr.bat")
        end
    end)
