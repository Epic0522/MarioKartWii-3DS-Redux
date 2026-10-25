// Sample named joint world matrices of one BRRES model for every CHR0 clip.
// Attachments (Lakitu's rod at CON_ROD, signal/boards at the rod's ef_hook)
// follow these joints, as the retail ModelAttach calls do (USA 0x8071e0d4).
// Usage: node build/export_joints.mjs file.brres out.bin model joint[,joint...]
import fs from 'node:fs';
import {readPoseMetadata,PoseEvaluator} from './brres_pose.js';
import {parse} from '../vendor/noclip/src/rres/brres.js';
import ArrayBufferSlice from '../vendor/noclip/src/ArrayBufferSlice.js';
import AnimationController from '../vendor/noclip/src/AnimationController.js';
const data=fs.readFileSync(process.argv[2]);
const archive=parse(new ArrayBufferSlice(data.buffer,data.byteOffset,data.byteLength));
const metadata=readPoseMetadata(data);
const model=archive.mdl0.find(m=>m.name===process.argv[4]);
if(!model)throw new Error('Missing model '+process.argv[4]);
const joints=process.argv[5].split(',').map(name=>{const n=model.nodes.find(x=>x.name===name);if(!n)throw new Error('Missing joint '+name);return n;});
const controller=new AnimationController();
const chunks:Buffer[]=[];const u32=(...v:number[])=>{const b=Buffer.alloc(v.length*4);v.forEach((x,i)=>b.writeUInt32LE(x,i*4));chunks.push(b);};
const clips=[null,...archive.chr0];
chunks.push(Buffer.from('DJNT'));u32(1,joints.length,clips.length);
for(const j of joints){const b=Buffer.alloc(32);b.write(j.name);chunks.push(b);}
const write=(m:Float32Array|number[])=>{const b=Buffer.alloc(48);[m[0],m[4],m[8],m[12],m[1],m[5],m[9],m[13],m[2],m[6],m[10],m[14]].forEach((v,i)=>{if(!Number.isFinite(v))throw new Error('Nonfinite joint');b.writeFloatLE(v,i*4);});chunks.push(b);};
for(const clip of clips){
 const b=Buffer.alloc(32);b.write(clip?clip.name:'bind');chunks.push(b);
 const duration=clip?Math.max(1,clip.duration):1;u32(duration,clip?clip.loopMode:0);
 const evaluator=new PoseEvaluator(metadata,model,clip,controller);
 for(let frame=0;frame<duration;frame++){
  controller.setTimeInFrames(frame);
  const matrices=evaluator.calculate();
  for(const j of joints){const m=matrices[j.mtxId];if(!m)throw new Error('Unevaluated joint '+j.name);write(m as Float32Array);}
 }
}
fs.writeFileSync(process.argv[3],Buffer.concat(chunks));
console.log(JSON.stringify({model:model.name,joints:joints.map(j=>j.name),clips:clips.map(c=>c?{name:c.name,duration:c.duration}:{name:'bind',duration:1})}));
