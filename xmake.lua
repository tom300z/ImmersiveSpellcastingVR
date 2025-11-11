-- set minimum xmake version
set_xmakever("2.8.2")

-- set config options before including commonlib
set_config("skyrim_se", false)
set_config("skyrim_ae", false)
set_config("skyrim_vr", true)

includes("lib/commonlibsse-ng")

-- set project
set_project("ImmersiveSpellcastingVR")
set_version("1.0.0")
set_license("MIT")

-- set defaults
set_languages("c++23")
set_warnings("allextra")

-- set policies
set_policy("package.requires_lock", true)

-- add rules
add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

-- Papyrus rule
rule("papyrus")
    set_extensions(".psc")
    on_build_files(function (target, sourcebatch, opt)
        import("core.project.depend")
        depend.on_changed(function ()
            os.exec("scripts\\compile_papyrus_mo2.cmd " .. "\"" .. path.join(os.projectdir(), target:targetdir()) .. "\"")
            print("Compiled papyrus")
        end, {files = sourcebatch.sourcefiles})
    end)

    on_install(function (target)
        os.cp(path.join(target:targetdir(), "scripts"), path.join(target:installdir(), "scripts"))
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
            local animation_path = path.relative(sourcefile, path.join(os.projectdir(), "mod_data_source"))
            local outfile = path.join(path.join(os.projectdir(), target:targetdir()), animation_path):gsub("%.%w+$", ".hkx")
            os.exec("hkxc.exe convert -v amd64 --input \"" .. sourcefile .. "\" --output \"" .. outfile .. "\"")
        end, {files = sourcefile})
    end)

    on_install(function (target)
        os.cp(path.join(target:targetdir(), "meshes"), path.join(target:installdir(), "meshes"))
    end)

-- targets
target("ImmersiveSpellcastingVR")
    -- add dependencies to target
    add_deps("commonlibsse-ng")

    -- add commonlibsse-ng plugin
    add_rules("commonlibsse-ng.plugin", {
        name = "ImmersiveSpellcastingVR",
        author = "tom300z",
        description = "SKSEVR Plugin that makes VR Spellcasting more immersive.",
        options = {
          address_library = true
        }
    })

    -- cpp
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
    before_build(function(target)
        -- Ensure install dir exists
        os.mkdir(target:installdir())

        -- Kill existing process before copying files
        if is_mode("debug") then
            os.exec("scripts\\kill_skyrimvr.bat")
        end
    end)
    after_build(function(target)
        -- Copy static mod data (esp, assets, etc)
        os.cp("mod_data_static/*", target:installdir())

        -- Ensure the new dll is always copied so it matches the symbols
        os.cp(target:targetfile(), path.join(target:installdir(), "SKSE\\Plugins"))

        -- (Re)start SkyrimVR if debugging
        if is_mode("debug") then
            os.exec("scripts\\kill_skyrimvr.bat")
            os.exec("scripts\\start_skyrimvr.bat")
        end
    end)
