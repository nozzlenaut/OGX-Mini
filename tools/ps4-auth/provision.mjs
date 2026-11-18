// Local-only RP2040 Zero (2 MiB hybrid) credential image creator.
// No network, dependency, key storage or telemetry.
export const ADDRESS = 0x101fb000;
export const SECTOR_SIZE = 4096;
export function crc32(bytes) {
  let c = 0xffffffff;
  for (const b of bytes) {
    c ^= b;
    for(let j=0;j<8;j++) c=(c>>>1)^((c&1)?0xedb88320:0);
  }
  return (~c)>>>0;
}
const concat=(...a)=>{const o=new Uint8Array(a.reduce((n,b)=>n+b.length,0));let i=0;for(const b of a){o.set(b,i);i+=b.length;}return o;};
const der=(tag,b)=>{
  let n=b.length;const l=[];
  if(n<128)l.push(n);else {const r=[];while(n){r.unshift(n&255);n>>>=8;}l.push(128|r.length,...r);}
  return concat(Uint8Array.of(tag,...l),b);
};
const decode=s=>Uint8Array.from(atob(s),c=>c.charCodeAt(0));
function pkcs8(pem) {
  const match=pem.trim().match(/^-----BEGIN (RSA PRIVATE KEY|PRIVATE KEY)-----\s*([A-Za-z0-9+/=\s]+)\s*-----END \1-----$/);
  if(!match)throw Error("Choose an unencrypted RSA private key (PEM, PKCS#1 or PKCS#8).");
  let bytes=decode(match[2].replace(/\s/g,""));
  if(match[1]==="RSA PRIVATE KEY") {
    const alg=Uint8Array.of(0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x01,0x05,0);
    bytes=der(0x30,concat(Uint8Array.of(2,1,0),alg,der(4,bytes)));
  }
  return bytes;
}
function component(s,length) {
  let b=decode(s.replace(/-/g,"+").replace(/_/g,"/").padEnd(Math.ceil(s.length/4)*4,"="));
  if(b.length>length)throw Error("Unsupported RSA component size.");
  const o=new Uint8Array(length);o.set(b,length-b.length);return o;
}
export async function credentialSector(pem,serialText,sig,subtle=globalThis.crypto.subtle) {
  const serial=serialText.trim();
  if(!/^[0-9a-f]{32}$/i.test(serial))throw Error("serial.txt must contain exactly 32 hexadecimal characters.");
  if(sig.length!==256 || sig.every(b=>b===0) || sig.every(b=>b===255))throw Error("sig.bin must be a nonempty 256-byte device signature.");
  const key=await subtle.importKey("pkcs8",pkcs8(pem),{name:"RSA-PSS",hash:"SHA-256"},true,["sign"]);
  if(key.algorithm.modulusLength!==2048)throw Error("RSA-2048 key required.");
  const jwk=await subtle.exportKey("jwk",key);
  const challenge=new Uint8Array(32);
  const pub=await subtle.importKey("jwk",{kty:"RSA",n:jwk.n,e:jwk.e,ext:true},{name:"RSA-PSS",hash:"SHA-256"},false,["verify"]);
  const trial=await subtle.sign({name:"RSA-PSS",saltLength:32},key,challenge);
  if(!await subtle.verify({name:"RSA-PSS",saltLength:32},pub,trial,challenge))throw Error("RSA self-test failed.");
  const sector=new Uint8Array(SECTOR_SIZE);sector.fill(255);sector.fill(0,0,1024);
  sector.set([79,71,65,49],0);
  sector.set(component(jwk.n,256),8);sector.set(component(jwk.e,4),264);
  sector.set(component(jwk.p,128),268);sector.set(component(jwk.q,128),396);
  sector.set(Uint8Array.from(serial.match(/../g),s=>parseInt(s,16)),524);sector.set(sig,540);
  new DataView(sector.buffer).setUint32(4,crc32(sector.subarray(8,1024)),true);
  return sector;
}
export function uf2(sector) {
  if(sector.length!==SECTOR_SIZE)throw Error("Invalid sector length.");
  const out=new Uint8Array(16*512), view=new DataView(out.buffer);
  for(let i=0;i<16;i++){
    const offset=i*512;
    [0x0a324655,0x9e5d5157,0x2000,ADDRESS+i*256,256,i,16,0xe48bff56].forEach((x,k)=>view.setUint32(offset+4*k,x,true));
    out.set(sector.subarray(i*256,(i+1)*256),offset+32);
    view.setUint32(offset+508,0x0ab16f30,true);
  }
  return out;
}
export function eraseImage(){return uf2(new Uint8Array(SECTOR_SIZE).fill(255));}
