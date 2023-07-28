return Module;
};
(function tryToExport(root, factory) {
    if (typeof exports === 'object' && typeof module === 'object')
      module.exports = factory();
    else if (typeof define === 'function' && define.amd)
      define("fwgsl", [], factory);
    else if (typeof exports === 'object')
      exports["fwgsl"] = factory();
    else      root["fwgsl"] = factory();
  })(typeof self !== "undefined" ? self : typeof global !== "undefined" ? global : this, () => {
    const initialize = (wasmPath) => {
      wasmPath = wasmPath || 'fwgsl.wasm'
      return new Promise(resolve => {
          Module({
              locateFile() {
                  return wasmPath;
              },
              onRuntimeInitialized() {
                  var fwgsl = this;
                  var wgsl = "";
                  var textDecoder = new TextDecoder();
                  var convertSpirV2WGSL = (code) => {
                    if (!fwgsl._return_string_callback) {
                        fwgsl._return_string_callback = (data, length) => {
                            const bytes = new Uint8ClampedArray(fwgsl.HEAPU8.subarray(data, data + length));
                            wgsl = textDecoder.decode(bytes);
                        };
                    }
                    let addr = fwgsl._malloc(code.byteLength);
                    fwgsl.HEAPU32.set(code, addr / 4);
                    fwgsl._spirv_to_wgsl(addr, code.byteLength);
                    fwgsl._free(addr);
                    return wgsl;
                  };
                  var entries = [];
                  var convertWGSL2SpirV = (code) => {
                      entries.splice(0, entries.length);
                      if (!fwgsl._return_entrypoint_callback) {
                          fwgsl._return_entrypoint_callback = (stage, data, length) => {
                              const buffer = fwgsl.HEAPU8.subarray(data, data + length);
                              entries.push(stage, buffer);
                          };
                      }
                      let addr = fwgsl._malloc(code.length);
                      fwgsl.HEAPU32.set(code, addr / 4);
                      fwgsl._wgsl_to_spirv(addr, code.length);
                      fwgsl._free(addr);
                      return entries;
                  };
                  resolve({
                      convertSpirV2WGSL: convertSpirV2WGSL,
                      convertWGSL2SpirV: convertWGSL2SpirV
                  });
              },
          });
      });
    };
    let instance;
    return (wasmPath) => {
        if (!instance) {
            instance = initialize(wasmPath);
        }
        return instance;
    };
});
