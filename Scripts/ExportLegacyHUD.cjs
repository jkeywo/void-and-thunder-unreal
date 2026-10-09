// Development-only raster export of the read-only original HUD panel art.
// No browser or HTML runtime is used by the Unreal game.
const fs = require('node:fs');
const path = require('node:path');
const {chromium} = require(process.env.VT_PLAYWRIGHT || 'playwright');
(async () => {
 const source=process.argv[2]; if(!source) throw new Error('Pass the source hud.html path');
 const out=path.resolve(__dirname,'../SourceAssets/ui'); fs.mkdirSync(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.VT_CHROMIUM});
 const page=await browser.newPage({viewport:{width:1440,height:900},deviceScaleFactor:2});
 await page.addInitScript(()=>{window.__hosted=true;let seed=137;Math.random=()=>{seed=(seed*1664525+1013904223)>>>0;return seed/4294967296;};});
 await page.goto('file:///'+path.resolve(source).replaceAll('\\','/'));
 await page.evaluate(table=>window.__applyStrings(JSON.stringify(table)),JSON.parse(fs.readFileSync(path.resolve(path.dirname(source),'../strings/en.json'),'utf8')));
 await page.evaluate(()=>window.updateHud({screen:'playing',hull:1,boostBattery:1,aimBattery:1,coords:{x:0,y:0},portCd:{remaining:0,duration:10},starboardCd:{remaining:0,duration:10},microwarpCd:{remaining:0,duration:20},wave:1,enemiesRemaining:3,plunder:0}));
 await page.addStyleTag({content:'*{animation:none!important;transition:none!important}'});
 await page.screenshot({path:path.join(out,'original-flight.png'),omitBackground:true});
 await page.evaluate(()=>window.updateHud({screen:'start'}));
 await page.screenshot({path:path.join(out,'original-menu.png'),omitBackground:true});
 await page.evaluate(()=>window.updateHud({screen:'playing'}));
 await page.addStyleTag({content:'.panel{filter:none!important}.glow,.scanband{visibility:hidden!important}'});
 for(const name of ['pTop','pStatus','pCoords','pLeft','pRight']) await page.locator('#'+name).screenshot({path:path.join(out,name+'.png'),omitBackground:true});
 fs.writeFileSync(path.join(out,'SOURCE.md'),'Panel artwork exported from the MIT-licensed original crates/vt_client/assets/ui/hud.html. Static metal/CRT layers only; native Unreal draws live gameplay readouts. Export script uses seeded cosmetic randomness. Original repository remains unchanged.\nSource commit: c138f2c9caab77ed8288ddcb46d1622e471c2b15.\nDroid Sans Mono is packaged from the installed engine distribution as a native composite font. Digitized data copyright 2006 Google Corporation.; Apache License 2.0 is retained in DroidSansMono-LICENSE.txt.\n');
 await browser.close();
})();
