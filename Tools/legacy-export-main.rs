fn main() {
 let data_root = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("assets/data");
 let read = |name: &str| std::fs::read_to_string(data_root.join(name)).unwrap();
 let ships: data::ships::ShipTable = ron::from_str(&read("ships.ron")).unwrap();
 let loadouts: data::loadouts::LoadoutCatalogue = ron::from_str(&read("loadouts.ron")).unwrap();
 let tuning: vt_sim::tuning::SimTuning = ron::from_str(&read("sim.tuning.ron")).unwrap();
 let feel: data::FeelTuning = ron::from_str(&read("feel.tuning.ron")).unwrap();
 let skirmish: data::scenario::Scenario = ron::from_str(&read("scenarios/skirmish.scn.ron")).unwrap();
 let test_range: data::scenario::Scenario = ron::from_str(&read("scenarios/test_range.scn.ron")).unwrap();
 let world = openworld::world_map::WorldMap::default();
 let baseline = serde_json::json!({"source_commit":"c138f2c9caab77ed8288ddcb46d1622e471c2b15", "ships":ships,"loadouts":loadouts,"tuning":tuning,"feel":feel,"world":world,"skirmish":skirmish,"test_range":test_range});
 std::fs::write(std::env::args().nth(1).expect("output JSON path"), serde_json::to_string_pretty(&baseline).unwrap()).unwrap();
}
