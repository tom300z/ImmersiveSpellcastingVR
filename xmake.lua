-- set minimum xmake version
set_xmakever("2.8.2")

-- set config options before including commonlib
set_config("skyrim_se", false)
set_config("skyrim_ae", false)
set_config("skyrim_vr", true)
set_config("rex_ini", true)

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

    local papyrus_targetdir = function (target) return path.join(os.projectdir(), target:targetdir(), "papyrus") end

    on_build_files(function (target, sourcebatch, opt)
        local build = function ()
            os.mkdir(papyrus_targetdir(target))
            os.exec("scripts\\compile_papyrus_mo2.cmd " .. "\"" .. papyrus_targetdir(target) .. "\"")
        end

        -- Build on changed files or missing target dir
        import("core.project.depend")
        depend.on_changed(build, {files = sourcebatch.sourcefiles})
        if not os.exists(papyrus_targetdir(target)) then
            build()
        end

    end)

    on_install(function (target)
        -- copy papyrus data to mod dir
        os.cp(path.join(papyrus_targetdir(target), "*"), target:installdir())
    end)

-- hkx animation rule
rule("hkx-anim")
    set_extensions(".xml")

    local hkx_targetdir = function (target) return path.join(os.projectdir(), target:targetdir(), "hkx-anim") end

    on_build_file(function (target, sourcefile, opt)
        if not sourcefile:match(".-[/\\]animations[/\\].+%.xml$") then
            -- Not an animation, don't process
            return
        end

        local animation_path = path.relative(sourcefile, path.join(os.projectdir(), "mod_data_source"))
        local targetfile = path.join(hkx_targetdir(target), animation_path):gsub("%.%w+$", ".hkx")

        local build = function ()
            -- build the file
            os.exec("hkxc.exe convert -v amd64 --input \"" .. sourcefile .. "\" --output \"" .. targetfile .. "\"")
        end

        -- Build on changed file or missing target file
        import("core.project.depend")
        depend.on_changed(build, {files = sourcefile})
        if not os.exists(targetfile) then
            build()
        end
    end)

    on_install(function (target)
        os.cp(path.join(hkx_targetdir(target), "*"), target:installdir())
    end)

-- targets
target("ISPVR")
    -- add dependencies to target
    add_deps("commonlibsse-ng")

    -- add commonlibsse-ng plugin
    add_rules("commonlibsse-ng.plugin", {
        name = "ISPVR - Immersive Spellcasting VR",
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
    on_config(function(target)
        -- Set installdir
        target:set("installdir", path.join(path.directory(target:installdir()), "ISPVR - Immersive Spellcasting VR"))
    end)

    before_build(function(target)
        -- Ensure install dir exists
        os.mkdir(target:installdir())

        -- Kill existing process before copying files
        if is_mode("debug") then
            os.exec("scripts\\kill_skyrimvr.bat")
        end
    end)

    before_install(function(target)
        -- Clear installdir
        os.rm(target:installdir())
        os.mkdir(target:installdir())
    end)

    after_install(function(target)
        -- Copy static mod data (esp, assets, etc)
        os.cp("mod_data_static/*", target:installdir() .. path.sep())

        -- Copy dll & symbols
        os.cp(target:targetfile(), path.join(target:installdir(), "SKSE\\Plugins") .. path.sep())
        local pdbpath = target:targetfile():gsub("%.%w+$", "") .. ".pdb"
        if os.exists(pdbpath) then
            os.cp(pdbpath, path.join(target:installdir(), "SKSE\\Plugins") .. path.sep())
        end

        -- (Re)start SkyrimVR if debugging
        if is_mode("debug") then
            os.exec("scripts\\kill_skyrimvr.bat")
            os.exec("scripts\\start_skyrimvr.bat")
        end

        -- create ZIP release (gotta move this to build at some point)
        if is_mode("releasedbg") then
            os.execv("powershell", {
                "-NoProfile", "-Command",
                "Compress-Archive -Path \""..target:installdir().."\\*\" -DestinationPath \""..path.join(target:installdir(), path.filename(target:installdir())..".zip").."\" -Force"
            })
        end
    end)
