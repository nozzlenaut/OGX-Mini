import test from "node:test";
import assert from "node:assert/strict";
import {generateKeyPairSync,randomBytes,webcrypto,createPrivateKey} from "node:crypto";
import {readFileSync} from "node:fs";
import {credentialSector,uf2,eraseImage,crc32,ADDRESS} from "../tools/ps4-auth/provision.mjs";

const pair=generateKeyPairSync("rsa",{modulusLength:2048,publicExponent:65537});
const pem8=pair.privateKey.export({type:"pkcs8",format:"pem"});
const pem1=pair.privateKey.export({type:"pkcs1",format:"pem"});
const serial="00112233445566778899aabbccddeeff";
const shortSerial="8899aabbccddeeff";
const sig=randomBytes(256); // synthetic certificate bytes; NOT real credentials
const build=pem=>credentialSector(pem,serial,sig,webcrypto.subtle);
test("PKCS#1 and PKCS#8 generate the same layout, including prime factors",async()=>{
 const [a,b]=await Promise.all([build(pem1),build(pem8)]);
 assert.deepEqual(a,b);assert.equal(Buffer.from(a.subarray(0,4)).toString(),"OGA1");
 assert.equal(new DataView(a.buffer).getUint32(4,true),crc32(a.subarray(8,1024)));
 assert.deepEqual(Buffer.from(a.subarray(524,540)),Buffer.from(serial,"hex"));
 assert.deepEqual(Buffer.from(a.subarray(540,796)),sig);
 const jwk=createPrivateKey(pem8).export({format:"jwk"});
 assert.deepEqual(Buffer.from(a.subarray(8,264)),Buffer.from(jwk.n,"base64url"));
 assert.deepEqual(Buffer.from(a.subarray(268,396)),Buffer.from(jwk.p,"base64url"));
 assert.deepEqual(Buffer.from(a.subarray(396,524)),Buffer.from(jwk.q,"base64url"));
});
test("GP2040-CE 16-character serial is left-padded to 16-byte identity",async()=>{
 const a=await credentialSector(pem8,shortSerial,sig,webcrypto.subtle);
 const padded=shortSerial.padStart(32,"0");
 const b=await credentialSector(pem8,padded,sig,webcrypto.subtle);
 assert.deepEqual(a,b);
 assert.deepEqual(Buffer.from(a.subarray(524,540)),Buffer.from(padded,"hex"));
});
test("bad key, serial, signature and wrong RSA size are refused",async()=>{
 await assert.rejects(build("not a key"));
 await assert.rejects(credentialSector(pem8,"bad",sig,webcrypto.subtle));
 await assert.rejects(credentialSector(pem8,serial,new Uint8Array(255),webcrypto.subtle));
 await assert.rejects(credentialSector(pem8,serial,new Uint8Array(256),webcrypto.subtle));
 const wrong=generateKeyPairSync("rsa",{modulusLength:1024}).privateKey.export({type:"pkcs8",format:"pem"});
 await assert.rejects(build(wrong));
});
test("UF2 is confined to the reserved sector; erase is all FF",async()=>{
 const sector=await build(pem8), image=uf2(sector), erased=eraseImage();
 const v=new DataView(image.buffer);
 assert.equal(image.length,8192);
 for(let i=0;i<16;i++){
  const p=i*512;
  assert.equal(v.getUint32(p,true),0x0a324655);
  assert.equal(v.getUint32(p+12,true),ADDRESS+256*i);
  assert.equal(v.getUint32(p+16,true),256);
  assert.equal(v.getUint32(p+20,true),i);
  assert.equal(v.getUint32(p+24,true),16);
  assert.equal(v.getUint32(p+28,true),0xe48bff56);
  assert.equal(v.getUint32(p+508,true),0x0ab16f30);
  assert.deepEqual(image.subarray(p+32,p+288),sector.subarray(i*256,(i+1)*256));
  assert.ok(erased.subarray(p+32,p+288).every(x=>x===255));
 }
});
test("offline HTML embeds the exact tested module and forbids network access",()=>{
 const html=readFileSync(new URL("../docs/ps4-auth.html",import.meta.url),"utf8");
 const source=readFileSync(new URL("../tools/ps4-auth/provision.mjs",import.meta.url),"utf8").replace(/^export /gm,"");
 assert.ok(html.includes(source)); assert.ok(html.includes("connect-src 'none'"));assert.ok(!html.includes('src="'));
});
