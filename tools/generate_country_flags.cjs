// Offline deterministic asset conversion. Requires Playwright and Chromium.
// Vendored SVG source/license/provenance are in assets/images/country-flags.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const {chromium}=require(process.env.PLAYWRIGHT_MODULE||'playwright');
const root=path.resolve(__dirname,'..'),dir=path.join(root,'assets/images/country-flags');
const countries=JSON.parse(fs.readFileSync(path.join(dir,'countries.json'),'utf8'));
const normalize=s=>s.toLowerCase().replace(/[^a-z0-9]/g,'');
(async()=>{
 const browser=await chromium.launch({executablePath:process.env.CHROMIUM_PATH||'/usr/bin/chromium',headless:true,args:['--no-sandbox']});
 let pixels;
 try {
  const page=await browser.newPage();await page.goto('about:blank');
  const sources=countries.map(c=>fs.readFileSync(path.join(dir,'source',c.code+'.svg'),'utf8'));
  pixels=await page.evaluate(async sources=>{
   const result=[],canvas=document.createElement('canvas');canvas.width=16;canvas.height=12;
   const context=canvas.getContext('2d',{willReadFrequently:true});
   for(const svg of sources){
    const image=new Image();image.src='data:image/svg+xml;charset=utf-8,'+encodeURIComponent(svg);
    await image.decode();
    context.fillStyle='#0b1d30';context.fillRect(0,0,16,12);context.drawImage(image,0,0,16,12);
    const rgba=context.getImageData(0,0,16,12).data,values=[];
    for(let i=0;i<rgba.length;i+=4)values.push((rgba[i]>>3)<<11|(rgba[i+1]>>2)<<5|rgba[i+2]>>3);
    result.push(values);
   }
   return result;
  },sources);
 } finally {await browser.close();}
 const byCode=new Map(countries.map((c,i)=>[c.code,i])),aliases=new Map();
 const add=(name,code)=>{
  const i=byCode.get(code);if(i===undefined)throw Error('Unknown code '+code);
  for(const v of [name,name.normalize('NFKD').replace(/[^\x00-\x7f]/g,'')]) {
   const key=normalize(v);if(!key)continue;
   if(!aliases.has(key)||aliases.get(key)===i)aliases.set(key,i);else aliases.set(key,null);
  }
 };
 for(const c of countries){add(c.code,c.code);add(c.name,c.code);}
 // Match existing model labels, English country variants and RadioID names.
 const model=fs.readFileSync(path.join(root,'main/bm_protocol.c'),'utf8');
 for(const m of model.matchAll(/\{"([A-Z]{2})","([^"]+)","[^\"]+"\}/g))add(m[2],m[1].toLowerCase());
 const extra={
  'United States':'us',USA:'us','United States of America':'us',
  UK:'gb','Great Britain':'gb',Britain:'gb',England:'gb-eng',Scotland:'gb-sct',Wales:'gb-wls','Northern Ireland':'gb-nir',
  'Russian Federation':'ru',Russia:'ru',Czechia:'cz','Czech Republic':'cz',
  'Hong Kong SAR':'hk','Hong Kong SAR China':'hk',Hongkong:'hk',
  China:'cn','PR China':'cn','Peoples Republic of China':'cn',
  Taiwan:'tw','Taiwan Province of China':'tw',
  'Korea Republic of':'kr','Republic of Korea':'kr','South Korea':'kr',
  'North Korea':'kp','Korea Democratic Peoples Republic of':'kp',
  Turkey:'tr',Turkiye:'tr',Türkiye:'tr','Viet Nam':'vn',Vietnam:'vn',
  'Bosnia-Herzegovina':'ba','Bosnia and Herzegovina':'ba','North Macedonia':'mk',Macedonia:'mk',
  Iran:'ir','Iran Islamic Republic of':'ir',Syria:'sy','Syrian Arab Republic':'sy',
  Laos:'la','Lao Peoples Democratic Republic':'la',Moldova:'md','Moldova Republic of':'md',
  Venezuela:'ve',Bolivia:'bo',Tanzania:'tz','Tanzania United Republic of':'tz',
  'Republic of the Congo':'cg','Democratic Republic of the Congo':'cd','DR Congo':'cd',
  'Ivory Coast':'ci',"Cote d'Ivoire":'ci','Cape Verde':'cv','Cabo Verde':'cv',
  'Brunei Darussalam':'bn',Palestine:'ps','Palestine State of':'ps',Swaziland:'sz',Eswatini:'sz',
  'East Timor':'tl','Timor-Leste':'tl','Holy See':'va','Vatican City':'va',
  'Reunion':'re',Curacao:'cw','Aland Islands':'ax','Saint Martin French part':'mf',
 };
 for(const [name,code]of Object.entries(extra))add(name,code);
 // Stored country labels are bounded to 23 UTF-8 bytes; add only unambiguous
 // truncated spellings. Never guess from a prefix shared by two countries.
 for(const c of countries) add(Buffer.from(c.name).subarray(0,23).toString('utf8'),c.code);
 for(const [name,code]of Object.entries(extra))add(Buffer.from(name).subarray(0,23).toString('utf8'),code);
 const pairs=[...aliases].filter(([_,i])=>i!==null).sort(([a],[b])=>a<b?-1:a>b?1:0);
 let data='/* Generated from vendored flag-icons 7.6.0 (MIT). See LICENSE/provenance.json. */\n#include "qso_flags.h"\n#include <string.h>\n\n';
 data+='const uint16_t qso_flag_pixels[][QSO_FLAG_WIDTH*QSO_FLAG_HEIGHT]={\n';
 for(let i=0;i<countries.length;i++)data+=' /* '+countries[i].code+' */ {\n'+Array.from({length:12},(_,y)=>'  '+pixels[i].slice(y*16,y*16+16).map(v=>'0x'+v.toString(16).padStart(4,'0')).join(',')).join(',\n')+'\n },\n';
 data+='};\nconst qso_flag_t qso_country_flags[]={\n'+countries.map((c,i)=>' {'+JSON.stringify(c.code)+','+JSON.stringify(c.name)+',qso_flag_pixels['+i+']},').join('\n')+'\n};\n';
 data+='const size_t qso_country_flag_count=sizeof(qso_country_flags)/sizeof(*qso_country_flags);\n';
 data+='static const struct {const char *name; uint16_t index;} aliases[]={\n'+pairs.map(([name,i])=>' {'+JSON.stringify(name)+','+i+'},').join('\n')+'\n};\n';
 data+='const qso_flag_t *qso_flag_lookup_key(const char *key)\n{\n size_t lo=0,hi=sizeof(aliases)/sizeof(*aliases);\n while(lo<hi){size_t mid=lo+(hi-lo)/2;int order=strcmp(key,aliases[mid].name);\n  if(!order)return &qso_country_flags[aliases[mid].index];\n  if(order<0)hi=mid;else lo=mid+1;\n }\n return NULL;\n}\n';
 fs.writeFileSync(path.join(dir,'qso_flag_data.c'),data);
 let images='/* Generated read-only LVGL descriptors; flag-icons artwork is MIT. */\n#include "qso_flag_images.h"\nextern const uint16_t qso_flag_pixels[][QSO_FLAG_WIDTH*QSO_FLAG_HEIGHT];\n';
 images+='static const lv_image_dsc_t flag_images[]={\n'+countries.map((c,i)=>' { .header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_RGB565,.w=QSO_FLAG_WIDTH,.h=QSO_FLAG_HEIGHT,.stride=QSO_FLAG_WIDTH*2}, .data_size=QSO_FLAG_WIDTH*QSO_FLAG_HEIGHT*2, .data=(const uint8_t *)qso_flag_pixels['+i+'] }, /* '+c.code+' */').join('\n')+'\n};\n';
 images+='const lv_image_dsc_t *qso_flag_image(const char *country)\n{\n const qso_flag_t *flag=qso_flag_find(country);\n return flag?&flag_images[flag-qso_country_flags]:NULL;\n}\n';
 fs.writeFileSync(path.join(dir,'qso_flag_images.c'),images);
 fs.writeFileSync(path.join(dir,'generated.json'),JSON.stringify({width:16,height:12,color_format:'RGB565',flags:countries.length,pixel_bytes:countries.length*16*12*2,aliases:pairs.length,data_sha256:crypto.createHash('sha256').update(data).digest('hex')},null,2)+'\n');
 console.log(JSON.stringify({flags:countries.length,pixelBytes:countries.length*16*12*2,aliases:pairs.length}));
})().catch(e=>{console.error(e);process.exitCode=1;});
