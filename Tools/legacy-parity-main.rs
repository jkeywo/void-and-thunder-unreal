use bevy_math::Vec2;
use serde_json::{json, Value};
use vt_sim::components::{Helm, ShipStats};
fn main() {
 let args: Vec<String>=std::env::args().collect();
 let baseline:Value=serde_json::from_str(&std::fs::read_to_string(&args[1]).unwrap()).unwrap();
 let mut flights=Vec::new();
 for entry in baseline["ships"]["classes"].as_array().unwrap() {
  let stats:ShipStats=serde_json::from_value(entry["class"]["stats"].clone()).unwrap();
  for case in 0..8 {
   let initial_heading=[0.0,0.7,2.9,-2.9,1.5,-1.2,0.0,0.4][case];
   let initial_velocity=if case==6 {Vec2::new(200.0,-180.0)} else {Vec2::new(10.0*case as f32,-4.0*case as f32)};
   let mut heading=initial_heading;let mut omega=0.05*case as f32;let mut velocity=initial_velocity;let mut position=Vec2::ZERO;let mut checkpoints=Vec::new();let mut controls=Vec::new();
   for step in 0..640 {
    let quarter=step/160;
    let throttle=match (case+quarter)%5 {0=>1.0,1=>-1.0,2=>0.0,3=>0.35,_=>0.8};
    let turn=match (case+quarter)%5 {0=>0.0,1=>1.0,2=>-1.0,3=>0.25,_=>-0.6};
    if step%160==0 {controls.push(json!([throttle,turn,160]));}
    (heading,omega,velocity)=vt_sim::ship::helm_step(heading,omega,velocity,&stats,&Helm{throttle,turn},0.25,1.0/64.0);
    heading=vt_sim::util::wrap_angle(heading);position+=velocity/64.0;
    if (step+1)%80==0 {checkpoints.push(json!({"step":step+1,"position":[position.x,position.y],"velocity":[velocity.x,velocity.y],"heading":heading,"omega":omega}));}
   }
   flights.push(json!({"hull":entry["name"],"case":case,"initial":{"heading":initial_heading,"omega":0.05*case as f32,"velocity":[initial_velocity.x,initial_velocity.y]},"controls":controls,"checkpoints":checkpoints}));
  }
 }
 let mut beams=Vec::new();let mut shields=Vec::new();
 for heading in [-3.0f32,-1.5,0.0,0.7,3.0] {for angle in [-3.1f32,-2.0,-0.8,0.0,0.8,2.0,3.1] {
  let aim=Vec2::from_angle(angle);
  for port in [false,true] {for arc in [0.0f32,0.35,1.1780972] {let d=vt_sim::combat::broadside_direction(heading,port,Some(aim),arc);beams.push(json!({"heading":heading,"port":port,"aim":[aim.x,aim.y],"arc":arc,"direction":[d.x,d.y]}));}}
  for arcs in [1u8,2,4] {let a=vt_sim::shield::shield_arc(heading,Vec2::ZERO,aim,arcs);let index=match a {vt_sim::shield::ShieldArc::Fore=>0,vt_sim::shield::ShieldArc::Aft=>1,vt_sim::shield::ShieldArc::Port=>2,vt_sim::shield::ShieldArc::Starboard=>3};shields.push(json!({"heading":heading,"impact":[aim.x,aim.y],"arcs":arcs,"answer":index}));}
 }}
 let out=json!({"source_commit":"c138f2c9caab77ed8288ddcb46d1622e471c2b15","flight":flights,"broadside":beams,"shield":shields});
 std::fs::write(&args[2],serde_json::to_string_pretty(&out).unwrap()).unwrap();
 println!("Exported {} flight trajectories, {} beams and {} shield bearings",flights.len(),beams.len(),shields.len());
}
