// Convert native LVGL PPM screenshots to PNG without extra image dependencies.
const fs=require('fs'),zlib=require('zlib');
function crc32(b){let c=~0;for(const v of b){c^=v;for(let i=0;i<8;i++)c=(c>>>1)^((c&1)?0xedb88320:0);}return (c^~0)>>>0;}
function chunk(type,data){const t=Buffer.from(type),n=Buffer.alloc(4),c=Buffer.alloc(4);n.writeUInt32BE(data.length);c.writeUInt32BE(crc32(Buffer.concat([t,data])));return Buffer.concat([n,t,data,c]);}
for(const path of process.argv.slice(2)){
 const input=fs.readFileSync(path),start=input.indexOf('255\n')+4;
 if(start<4||input.length-start!==240*320*3)throw Error('Unexpected PPM '+path);
 const h=Buffer.alloc(13);h.writeUInt32BE(240);h.writeUInt32BE(320,4);h[8]=8;h[9]=2;
 const scan=Buffer.alloc((240*3+1)*320);
 for(let y=0;y<320;y++)input.copy(scan,y*721+1,start+y*720,start+(y+1)*720);
 const png=Buffer.concat([Buffer.from([137,80,78,71,13,10,26,10]),chunk('IHDR',h),chunk('IDAT',zlib.deflateSync(scan)),chunk('IEND',Buffer.alloc(0))]);
 fs.writeFileSync(path.replace(/\.ppm$/,'.png'),png);
}
