#!/usr/bin/env ejs

/*
    bit.es -- Build It! -- Embedthis Build It Framework
 */

module embedthis.bit {

require ejs.unix

class Bit 
{
    private const RC: String = '.bitrc'
    private const VERSION: Number = 0.1

    private var appName: String = 'bit'
    private var args: Args
    private var options: Object
    private var rest: Array
    private var currentBit: Path            // Name of the bit file being currently loaded
    private var currentComponent            // Name of the current component being loaded

    /*
        Aggregate Bit configuration. This has all bit files blended into one object.
     */
    private var spec: Object = { components: {} }

    private var targets: Array = []         // Ordered set of targets

    private var posix = ['MACOSX', 'LINUX', 'UNIX', 'FREEBSD', 'SOLARIS']
    private var windows = ['WIN', 'WINCE']
    private var start: Date

    function Bit() {
    }

    private var argTemplate = {
        options: {
            benchmark: { alias: 'b' },
            build: { range: String },
            config: { range: String },
            debug: {},
            init: { range: String },
            dev: {},
            host: {},
            init: { range: String },
            overwrite: {},
            release: {},
            save: { range: Path },
            show: {},
            verbose: { alias: 'v' },
            version: { alias: 'V' },
            why: {},
        },
        usage: usage
    }

    function usage(): Void {
        print('\nUsage: bit [options] [targets] ...\n' +
            '  Options:\n' + 
            '    --benchmark            # Measure elapsed time\n' +
            '    --build path           # Use the specified build.bit\n' +
            '    --config configuration # Use the build configuration\n' +
            '    --debug                # Debug the build\n' +
            '    --dev                  # Build the dev tools only\n' +
            '    --host                 # Build for the host only\n' +
            '    --init path-to-source  # Initialize for building\n' +
            '    --log logFile          # Send log output to a log file \n' +
            '    --overwrite            # Overwrite when generating files\n' +
            '    --save path            # Save aggregated bit file\n' +
            '    --show                 # Show commands executed\n' +
            '    --version              # Dispay the bit version\n' +
            '    --verbose              # Trace operations\n' +
            '')
        App.exit(1)
    }

    function main() {
        let start = new Date
        global._b = this
        global.spec = spec
        args = Args(argTemplate)
        options = args.options
        try {
            processOptions(args)
            process()
        } catch (e) {
            let msg: String
            if (e is String) {
                msg = e
                App.log.error('bit: Error: ' + msg + '\n')
            } else {
                msg = (options.debug) ? e : e.message
                App.log.error('bit: Error: ' + msg + '\n')
            }
            App.exit(2)
        }
        if (options.benchmark) {
            App.log.activity('Benchmark', "Elapsed time %.2f" % ((start.elapsed / 1000)) + " secs.")
        }
    }

    function processOptions(args: Args) {
        if (options.debug) {
            options.config = 'debug'
        }
        if (options.release) {
            options.config = 'release'
        }
        if (options.version) {
            print(version)
            App.exit(0)
        }
        if (options.verbose && !options.log) {
            options.log = 'stderr:1'
        }
        if (options.log) {
            App.log.redirect(options.log)
            App.mprLog.redirect(options.log)
        }
        currentBit = options.build
    }

    function getDevPlatform() {
        return 'x86_64-apple-macosx.bit'
    }

    function initialize() {
        let dev = getDevPlatform()
        //  MOB - 
        let platform = Path('bit').join(dev).joinExt('bit')
        loadWrapper(platform)
        loadWrapper('product.bit')
        trace('Init', spec.settings.title)

        expandTokens(spec)
        setTypes()
        findComponents()

        let nspec = { 
            '+blend' : [
                platform,
                'product.bit',
            ],
            '+settings': spec.settings,
            components: spec.components,
        }
        let bbit: Path = 'build.bit'
        if (bbit.exists && !options.overwrite) {
            throw 'The ' + bbit + ' file already exists. Use bit --overwrite'
        }
        trace('Generate', bbit)
        bbit.write('/*\n    build.bit -- Build It for ' + spec.settings.title + 
            '\n\n    Generated by bit.\n */\n\nbit(' + 
            serialize(nspec, {pretty: true, indent: 4, commas: true, quotes: false}) + ')\n')
    }

    function findComponents() {
        trace('Find', 'Components')
        let dir = Path(options.init)
        for each (component in spec.required + spec.optional) {
            let path = dir.join('bit/components', component + '.bit')
            vtrace('Find', 'Component ' + component)
            if (path.exists) {
                try {
                    spec.components[component] ||= {}
                    currentComponent = component
                    loadWrapper(path)
                } catch (e) {
                    if (!(e is String)) {
                        App.log.debug(0, e)
                    }
                    let kind = spec.required.contains(component) ? 'Required' : 'Optional'
                    whyMissing(kind + ' componenent "' + component + '" ' + e)
                    spec.components[component] = { diagnostic: "" + e }
                }
            } else {
                throw "Unknown component " + path
            }
        }
    }

    public function probe(file: Path, control = {}): Path {
        let path: Path
        if (!file.exists) {
            let search = []
            let dir
            if (dir = spec.components[currentComponent].path) {
                search.push(dir)
            }
            if (control.search && control.search is Array) {
                search += control.search
            }
            for each (s in search) {
                if (s.join(file).exists) {
                    path = s.join(file)
                    break
                }
            }
            path ||= Cmd.locate(file)
        }
        if (!path) {
            throw 'component not found'
        }
        if (control.fullpath) {
            return path
        }
        return path.toString().replace(RegExp('[/\\\\]' + path + '$'), '')
    }

    function process() {
        if (options.init) {
            initialize()
            return
        }
        if (!currentBit) {
            findConfig()
        }
        loadWrapper(currentBit)
        expandTokens(spec)

        let host = spec.host
        host.cross ||= (host.arch != Config.CPU || host.os != Config.OS)
        if (host.cross) {
            build(!host.cross)
        }
        build()
    }

    function loadWrapper(path) {
        let saveCurrent = currentBit
        currentBit = path
        vtrace('Loading', currentBit)
        load(path)
        currentBit = saveCurrent
    }

    public function loadBitfile(o) {
        let toBlend = blend({}, o, {combine: true})
        /* Blending is depth-first. So blend existing spec over blended spec */
        /* Load blended bit files first */
        for each (path in toBlend.blend) {
            loadWrapper(currentBit.dirname.join(path))
        }
        spec = blend(spec, o, {combine: true})
    }

    function findConfig() {
        let base: Path = currentBit || '.'
        for (let d: Path = base; d.parent != d; d = d.parent) {
            let f: Path = d.join('build.bit')
            if (f.exists) {
                currentBit = f
                return
            }
        }
        throw 'Can\'t locate build.bit'
    }

    function prepBuild(cross) {
        setTokens(cross)
        getOrderedTargets()
        setTypes()
        expandWildcards()
        blendCommon()
        setTypes()
        setTargetPaths()
        Object.sortProperties(spec);
        if (options.save) {
            spec.blended = spec.blend
            delete spec.blend
            options.save.write(serialize(spec, {pretty: true, commas: true, indent: 4, quotes: false}))
            trace('Save', options.save)
            App.exit()
        }
    }

    function getOrderedTargets() {
        if (args.rest.length > 0) {
            for each (target in args.rest) {
                orderTargets(target, spec.targets[target])
            }
        } else {
            for (let [name, target] in spec.targets) {
                if (target.type) {
                    orderTargets(name, target)
                }
            }
        }
        if (options.verbose) {
            let names = ''
            for each (target in targets) {
                names += target.name + ' '
            }
            trace('Targets', names)
        }
    }

    function orderTargets(tname, target) {
        if (target.depend) {
            for each (dname in target.depend) {
                let dep = spec.targets[dname]
                if (!dname) {
                    throw 'Unknown dependency "' + dname + '" in target "' + tname + '"'
                }
                if (!targets.contains(dep)) {
                    orderTargets(dname, dep)
                    targets.push(dep)
                    dep.name = dname
                }
            }
        }
        target.name = tname
        targets.push(target)
    }

    function setTargetPaths() {
        for each (target in targets) {
            if (!target.path) {
                if (target.type == 'lib') {
                    target.path = spec.directories.lib.join(target.name).joinExt(spec.extensions.shlib)
                } else if (target.type == 'obj') {
                    target.path = spec.directories.obj.join(target.name).joinExt(spec.extensions.obj)
                } else if (target.type == 'exe') {
                    target.path = spec.directories.bin.join(target.name).joinExt(spec.extensions.exe)
                } else {
                    target.path = target.name
                }
            }
        }
    }

    function buildFileList(include, exclude)
    {
        let files
        if (include is RegExp) {
            files = Path('.').glob('*', {include: include})
        } else if (include is Array) {
            files = []
            for each (pattern in include) {
                files += Path('.').glob(pattern)
            }
        } else {
            files = Path('.').glob(include)
        }
        if (exclude) {
            if (exclude is RegExp) {
                files = files.reject(function (elt) elt.match(exclude)) 
            } else if (exclude is Array) {
                for each (pattern in exclude) {
                    files = files.reject(function (elt) { return elt.match(pattern); } ) 
                }
            } else {
                files = files.reject(function (elt) elt.match(exclude))
            }
        }
        return files
    }

    /*
        Expand target.sources and target.headers. Support include+exclude and create target.files[]
     */
    function expandWildcards() {
        let index
        for (index = 0; index < targets.length; index++) {
            target = targets[index]
            if (target.headers) {
                let files = buildFileList(target.headers.include, target.headers.exclude)
                for each (file in files) {
                    //  Create a target for each header
                    let header = spec.directories.inc.join(file.basename)
                    //  MOB - do we need a files[], do we need build:
                    let newTarget = { name : file, path: header, type: 'header', files: [ file ] }
                    if (spec.targets[file]) {
                        newTarget = blend(spec.targets[file], newTarget, {combined: true})
                    } else {
                        spec.targets[newTarget.name] = newTarget
                    }
                    targets.insert(index++, newTarget)
                    // target.files.push(header)
                }
            }
            if (target.sources) {
                target.files = []
                let files = buildFileList(target.sources, target.exclude)
                for each (file in files) {
                    /*
                        Create a target for each source file
                     */
                    let obj = spec.directories.obj.join(file.replaceExt(spec.extensions.obj).basename)
                    //  MOB - do we need files[]
let includes = spec.common.includes
if (target.includes) {
    includes = includes + target.includes
}
                    let newTarget = { name : file, path: obj, type: 'obj', files: [ file ], 
                        includes: includes,
                        /* UNUSED, build: target.build */}
                    if (spec.targets[file]) {
                        newTarget = blend(spec.targets[file], newTarget, {combined: true})
                    } else {
                        spec.targets[newTarget.name] = newTarget
                    }
                    newTarget.depend = depend(newTarget)
                    targets.insert(index++, newTarget)
                    target.files.push(obj)
                }
            }
        }
    }

    function blendCommon() {
        let common = {}
        for (name in spec.common) {
            common['+' + name] = spec.common[name]
        }
        for each (target in targets) {
            if (target.type && target.type != 'header') {
                blend(target, common, {combine: true})
            }
        }
    }

    function setTypes() {
        for (let [key,value] in spec.directories) {
            spec.directories[key] = Path(value)
        }
    }

    function build(cross: Boolean) {
        prepBuild(cross)
        for each (target in targets) {
            buildTarget(target, cross)
        }
    }

    function buildTarget(target, cross: Boolean) {
        if (target.type == 'obj') {
            buildObj(target, cross)
        } else if (target.type == 'lib') {
            buildLib(target, cross)
        } else if (target.type == 'exe') {
            buildExe(target, cross)
        } else if (target.type == 'header') {
            //  MOB
        } else {
            throw 'Unknown target type in ' + target.path
        }
    }

    function buildExe(target, cross: boolean) {
        if (!stale(target, target.files)) {
            whySkip(target.path, 'is up to date')
            return
        }
        debug('Building:\n' + target.path + ' = ' + serialize(target, {pretty: true}))
        if (options.debug) {
            dump('TARGET', target)
        }
        let transition = 'EXE'
        let rule = spec.rules[transition]
        if (!rule) {
dump('FAILED', target)
            throw 'No rule to build target ' + target.path + ' for transition ' + transition
            return
        }
        spec.target = target
        spec.PREPROCESS = ''
        spec.OUT = target.path
        spec.IN = target.files.join(' ')
        spec.LIBS = target.libraries.map(function(e) '-l' + Path(e).trimExt().toString().replace(/^lib/, '') )
        spec.DEBUG = '-g'

        /* Double expand so rules tokens can use ${OUT} */
        let command = rule.expand(spec, {fill: ''})
        command = command.expand(spec, {fill: ''})
        trace('Link', target.name)
        debug(2, command)
        let run = runCmd(command)
        if (run.status != 0) {
            throw 'Build failure for ' + target.path + '\n' + run.error
        }
    }

    function buildLib(target, cross: boolean) {
        //  MOB - need libraries[] so we can stat them
        if (!stale(target, target.files)) {
            whySkip(target.path, 'is up to date')
            return
        }
        debug('Building:\n' + serialize(target, {pretty: true}))
        if (options.debug) {
            dump('TARGET', target)
        }
        let transition = 'LIB'
        let rule = spec.rules[transition]
        if (!rule) {
dump('FAILED', target)
            throw 'No rule to build target ' + target.path + ' for transition ' + transition
            return
        }
        spec.target = target
        spec.PREPROCESS = ''
        spec.OUT = target.path
        spec.IN = target.files.join(' ')
        spec.DEBUG = '-g'
        spec.LIBS = target.libraries.map(function(e) '-l' + Path(e).trimExt().toString().replace(/^lib/, '') )

        /* Double expand so rules tokens can use ${OUT} */
        let command = rule.expand(spec, {fill: ''})
        command = command.expand(spec, {fill: ''})
        trace('Link', target.name)
        let run = runCmd(command)
        if (run.status != 0) {
            throw 'Build failure for ' + target.path + '\n' + run.error
        }
    }

    function buildObj(target, cross: boolean) {
        if (!stale(target, target.files)) {
            return
        }
        debug('Building:\n' + serialize(target, {pretty: true}))
        if (options.debug) {
            dump('TARGET', target)
        }
        let ext = target.path.extension
        for each (file in target.files) {
            let transition = file.extension + '->' + target.path.extension
            let rule = spec.rules[transition]
            if (!rule) {
                rule = spec.rules[target.path.extension]
                if (!rule) {
                    dump('FAILED', target)
                    throw 'No rule to build target ' + target.path + ' for transition ' + transition
                    return
                }
            }
            //  MOB - rethink how this is done
            spec.target = target
            spec.PREPROCESS = ''
            spec.OUT = target.path
            spec.IN = file
            spec.DEBUG = '-g'
            spec.INCLUDES = (target.includes) ? spec.INCLUDES = target.includes.map(function(e) '-I' + e ) : ''
            spec.LIBS = target.libraries.map(function(e) '-l' + Path(e).trimExt().toString().replace(/^lib/, '') )

            let command = rule.expand(spec, {fill: ''})
            trace('Compile', file)
            let run = runCmd(command)
            if (run.status != 0) {
                throw 'Build failure for ' + target.path + '\n' + run.error
            }
        }
    }

    function stale(target, inputs: Array) {
        let path = target.path
        if (!path.modified) {
            whyRebuild(path, 'Rebuild', 'is missing.')
            return true
        }
        for each (file in inputs) {
            if (file.modified > path.modified) {
                whyRebuild(path, 'Rebuild', 'input ' + file + ' has been modified.')
                return true
            }
        }
        for each (let dep: Path in target.depend) {
            if (!spec.targets[dep]) {
                /* If dependency is not a target, then treat as a file */
                if (!dep.modified) {
                    whyRebuild(path, 'Rebuild', 'missing dependency ' + dep)
                    return true
                }
                if (dep.modified > path.modified) {
                    whyRebuild(path, 'Rebuild', 'dependency ' + dep + ' has been modified.')
                    return true
                }
                return false
            }
            let file = spec.targets[dep].path
            if (file.modified > path.modified) {
                whyRebuild(path, 'Rebuild', 'dependent ' + file + ' has been modified.')
                return true
            }
        }
        debug('Building:\n' + serialize(target, {pretty: true}))
        return false
    }

    function depend(target): Array {
        let includes: Array = []
        for each (path in target.files) {
            let str = path.readString()
            //  MOB - remove when array += null is a NOP
            let more = str.match(/^#include.*"/gm)
            if (more) {
                includes += more
            }
        }
        let depends = []
        for each (item in includes) {
            let ifile = item.replace(/#include.*"(.*)"/, '$1')
            let path
            for each (dir in target.includes) {
                dir = dir.replace('-I', '')
                path = Path(dir).join(ifile)
                if (path.exists && !path.isDir) {
                    break
                }
                path = null
            }
            if (path) {
                depends.push(path)
            } else {
                App.log.error('Can\'t find include file ' + ifile + ' for ' + target.name)
            }
        }
        return depends
    }

    function setTokens(cross: Boolean) {
        if (cross) {
            spec.ARCH = spec.host.arch
            spec.OS = spec.host.os
        } else {
            spec.OS = Config.OS
            spec.ARCH = Config.CPU
        }
        trace('Building', spec.settings.title + ' for ' + spec.OS + ', ' + spec.ARCH);
    }

    function expandTokens(o) {
        for (let [key,value] in o) {
            if (value is String) {
                o[key] = value.expand(spec, {fill: '${}'})
            } else if (Object.getOwnPropertyCount(value) > 0) {
                o[key] = expandTokens(value)
            }
        }
        return o
    }

    function runCmd(command: String): Cmd {
        if (options.show) {
            App.log.activity('Run', command)
        }
        let run = Cmd(command)
        return run
    }

    function trace(tag, msg) {
        App.log.activity(tag, msg)
    }

    function vtrace(tag, msg) {
        if (options.verbose) {
            App.log.activity(tag, msg)
        }
    }

    function whyRebuild(path, tag, msg) {
        if (options.why) {
            App.log.activity(tag, path + ' because ' + msg)
        }
    }

    function whySkip(path, msg) {
        if (options.why) {
            App.log.activity('Target', path + ' ' + msg)
        }
    }

    function whyMissing(msg) {
        if (options.why) {
            App.log.activity('Init', msg)
        }
    }

    function debug(msg) {
        if (options.debug) {
            App.log.activity('Debug', msg)
        }
    }
}

} /* bit module */


require embedthis.bit

public var b: Bit = new Bit
b.main()

public function bit(o: Object) {
    b.loadBitfile(o)
}

public function probe(file: Path, options = {}): Path {
    return b.probe(file, options)
}

public function program(name)
{
    let components = {}
    components[name] = { path: probe(name, {fullpath: true})}
    bit({components: components})
}

/*
    @copy   default
  
    Copyright (c) Embedthis Software LLC, 2003-2012. All Rights Reserved.
    Copyright (c) Michael O'Brien, 1993-2012. All Rights Reserved.
  
    This software is distributed under commercial and open source licenses.
    You may use the GPL open source license described below or you may acquire
    a commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.TXT distributed with
    this software for full details.
  
    This software is open source; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version. See the GNU General Public License for more
    details at: http://www.embedthis.com/downloads/gplLicense.html
  
    This program is distributed WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
    This GPL license does NOT permit incorporating this software into
    proprietary programs. If you are unable to comply with the GPL, you must
    acquire a commercial license to use this software. Commercial licenses
    for this software and support services are available from Embedthis
    Software at http://www.embedthis.com
  
    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */
