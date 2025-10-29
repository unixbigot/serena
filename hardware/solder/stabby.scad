/* [Settings] */

// Which part to print
part="assembly"; // [assembly,frame,carriage,motor_bracket,idler_bracket]

// distance of carriage movement 
stroke=50; 

// length of frame and carriage
frame_length=50;

// width of frame
frame_width=21;

// thickness of frame sides
frame_thickness=4;

// width of slot for belt clearance
frame_slot_length=15;

// height of the moving carriage
carriage_height=20;

// connector type for carriage-to-stabulator
carriage_connector_type = "quarterinch"; // ["quarterinch", "gopro"]

// connector type for holding the frame
frame_connector_type = "quarterinch"; // ["quarterinch", "gopro"]

/* [Advanced] */

// make the motor bracket part of the frame (otherwise separate)
integral_motor_bracket=false;

// make the idler pulley bracket part of the frame (otherwise separate)
integral_idler_bracket=false;


// NEMA size of stepper motor mounting
nema_size=17;

// distance to subtract from carriage width to allow sliding inside frame
carriage_clearance=1;

// diameter of carriage rails
rail_dia=5;

// inset from frame-edge of the rails
rail_inset=7.5;

// size of the bushes/bearings inset into the carriage
bush_height=6;
bush_dia=9;

// extra clearance for detached motor and idler brackets
detached_bracket_rail_clearance=1;

/* [Hidden] */

// carriage width is derived from the frame size and clearances
carriage_width = frame_width-frame_thickness-carriage_clearance;

// carriage length is derived from teh frame size and clearances
carriage_length = frame_length-frame_thickness-carriage_clearance;

include <BOSL/nema_steppers.scad>
include <BOSL/linear_bearings.scad>
include <BOSL/metric_screws.scad>
include <BOSL/shapes.scad>
include <gopro_mounts_mooncactus.scad>
include <lozenge.scad>

$fn=60;
fuz=0.1;fuzz=2*fuz;

frame_height = frame_thickness+carriage_height+stroke+frame_thickness;
motor_side = nema_motor_width(nema_size);

qi_depth=8;
qi_length=20;
qi_width=16;
qi_nut_thickness=4.5;
qi_nut_af = 11.4;
qi_nut_dia = 13;
qi_screw_dia = 6.4;

module quarterinch_connector() {
    difference() {
	cube([qi_length,qi_width,qi_depth]);
	translate([qi_length/2,qi_width/2,-fuz]) {
	    cylinder(d=qi_screw_dia, h=qi_depth+fuzz);
	    cylinder(d=qi_nut_dia, h=qi_nut_thickness+fuz, $fn=6);
	}
	translate([qi_length/2,(qi_width-qi_screw_dia)/2, -fuz]) cube([qi_length,qi_screw_dia,qi_depth+fuzz]);
	translate([qi_length/2,(qi_width-qi_nut_af)/2, -fuz]) cube([qi_length,qi_nut_af,qi_nut_thickness+fuz]);
    }
}

module motor_bracket(integral=true) {

    module motor_mount() {
	difference() {
	    halflozenge(motor_side,motor_side+10,frame_thickness,5);
	    translate([motor_side/2,(motor_side+10)/2,frame_thickness/2]) nema_mount_holes(size=nema_size,depth=frame_thickness+fuzz);
	}
    }
    if (integral) {
	motor_mount();
    }
    else {
	difference() {
	    union() {
		// mate with the upper end of the frame
		cube([frame_length,frame_thickness,frame_width]);
		// motor surround
		translate([(frame_length-frame_thickness-motor_side)/2,frame_thickness,0]) motor_mount();
		color("red") hull() {
		    cube([(frame_length-frame_thickness-motor_side)/2,frame_width,frame_thickness]);
		    cube([(frame_length-frame_thickness-motor_side)/2,frame_thickness,frame_width]);
		}
		translate([(frame_length-frame_thickness+motor_side)/2,0,0])
		color("red") hull() {
		    cube([(frame_length-frame_thickness-motor_side)/2+frame_thickness,frame_width,frame_thickness]);
		    cube([(frame_length-frame_thickness-motor_side)/2+frame_thickness,frame_thickness,frame_width]);
		}
		
	    }
	    // make a slot in the frame to allow the belt to pass through
	    translate([(frame_length-frame_thickness-frame_slot_length)/2,-fuz,frame_thickness]) cube([frame_slot_length,frame_thickness+fuzz, frame_width-frame_thickness+fuz]);

	    // rail holes
	    translate([rail_inset, -fuz, (frame_width+frame_thickness)/2]) rotate([-90,0,0]) cylinder(d=rail_dia+detached_bracket_rail_clearance,h=frame_thickness+fuzz);
	    translate([frame_length-frame_thickness-rail_inset, -fuz, (frame_width+frame_thickness)/2]) rotate([-90,0,0]) cylinder(d=rail_dia+detached_bracket_rail_clearance,h=frame_thickness+fuzz);
	}
    }
}

module idler_bracket(length=frame_length, integral=true) {
    module idler_mount() {
	difference() {
	    halflozenge(length,frame_width,frame_thickness,min(length,frame_width)/2);
	    translate([length/2,frame_width/2,frame_thickness/2]) cylinder(d=rail_dia, h=frame_thickness+fuzz,center=true);
	}
    }

    if (integral) {
	idler_mount();
    }
    difference() {
	union() {
	    // mate with the lower end of the frame
	    translate([0,0,frame_thickness-frame_width]) cube([frame_length,frame_thickness,frame_width]);
	    // idler mount
	    translate([0,0,0]) idler_mount();



	}
	// make a slot in the frame to allow the belt to pass through
	translate([(frame_length-frame_thickness-frame_slot_length)/2,-fuz,-frame_width]) cube([frame_slot_length,frame_thickness+fuzz, frame_width]);

	// rail holes
	translate([rail_inset, -fuz, -(frame_width-frame_thickness)/2]) rotate([-90,0,0]) cylinder(d=rail_dia+detached_bracket_rail_clearance,h=frame_thickness+fuzz);
	translate([frame_length-frame_thickness-rail_inset, -fuz, -(frame_width-frame_thickness)/2]) rotate([-90,0,0]) cylinder(d=rail_dia+detached_bracket_rail_clearance,h=frame_thickness+fuzz);
    }
}

module frame() {
    difference() {
	union() {
	    // the backbone of the frame
	    cube([frame_length,frame_width,frame_thickness]);

	    // the bottom end of the frame
	    translate([frame_length-frame_thickness,0,0]) cube([frame_thickness,frame_width,frame_height]);

	    // the upper end of the frame
	    translate([0,0,frame_height-frame_thickness]) cube([frame_length,frame_width,frame_thickness]);

	    // the right-hand side of the frame (left side is left open)
	    translate([0,frame_width-frame_thickness]) cube([frame_length,frame_thickness,frame_height]);

	}

	// subtract the left hand rail
	translate([rail_inset, (frame_width-frame_thickness)/2, -fuz]) cylinder(d=rail_dia,h=frame_height+fuzz);

	// subtract the right hand rail
	translate([frame_length-frame_thickness-rail_inset, (frame_width-frame_thickness)/2, -fuz]) cylinder(d=rail_dia,h=frame_height+fuzz);

	// make a slot in the frame to allow the belt to pass through
	translate([(frame_length-frame_thickness-frame_slot_length)/2,-fuz,-fuz]) cube([frame_slot_length,frame_width-frame_thickness,frame_height+fuzz]);

	if (frame_connector_type=="quarterinch") {
	    // tapping hole for 1/4WW = 5.1mm
	    translate([frame_length-frame_thickness/2,
		       frame_width/2,
		       frame_height/2
			  ])
		rotate([0,90,0]) cylinder(d=5.1, h=frame_thickness+fuzz,center=true);
	}
    }

    //  sketch in the left side rail in preview
    translate([rail_inset, (frame_width-frame_thickness)/2, -fuz]) color("silver") %cylinder(d=rail_dia,h=frame_height+fuzz);

    // sketch in the rihgt side rail in preview
    translate([frame_length-frame_thickness-rail_inset, (frame_width-frame_thickness)/2, -fuz]) color("silver") %cylinder(d=rail_dia,h=frame_height+fuzz);

    if (frame_connector_type == "gopro") {
	translate([frame_length+gopro_connector_y,
		   frame_width-gopro_connector_x/2,
		   frame_height/2
		      ])
	    rotate([0,0,90]) gopro_connector("double");
    }
	
    if (integral_motor_bracket) {
	translate([(frame_length-frame_thickness-motor_side)/2,frame_width,frame_height]) rotate([90,0,0]) motor_bracket();
    }
    if (integral_idler_bracket) {
	idler_bracket_length = frame_slot_length;
	translate([(frame_length-frame_thickness-idler_bracket_length)/2,frame_width-frame_thickness,0]) rotate([-90,0,0]) idler_bracket(length=idler_bracket_length);
    }
}

module carriage() {
    difference() {
	union() {
	    // body of the carriage
	    color("beige") cube([carriage_length,carriage_width,carriage_height]);
	    if (carriage_connector_type == "gopro") {
		// gopro connector
		translate([-gopro_connector_y,carriage_width/2, gopro_connector_x/2]) rotate([0,-90,-90]) gopro_connector("triple");
	    }
	    else {
		// cage for 1/4" whitworth bolt
		translate([0, (carriage_width-qi_width)/2,0]) rotate([0,-90,0]) quarterinch_connector();
	    }
	}

	// left rail
	translate([rail_inset, (frame_width-frame_thickness)/2, -fuz]) cylinder(d=rail_dia+carriage_clearance,h=frame_height+fuzz);
	// left lower bushing
	translate([rail_inset, (frame_width-frame_thickness)/2, -fuz]) cylinder(d=bush_dia,h=bush_height+fuz);
	// left lower bushing taper
	translate([rail_inset, (frame_width-frame_thickness)/2, bush_height]) sphere(d=bush_dia);
	// left upper bushing
	translate([rail_inset, (frame_width-frame_thickness)/2, carriage_height-bush_height]) cylinder(d=bush_dia,h=bush_height+fuz);

	// right rail
	translate([frame_length-frame_thickness-rail_inset, (frame_width-frame_thickness)/2, -fuz]) cylinder(d=rail_dia+carriage_clearance,h=carriage_height+fuzz);
	// right lower bushing
	translate([frame_length-frame_thickness-rail_inset, (frame_width-frame_thickness)/2, -fuz]) cylinder(d=bush_dia,h=bush_height+fuz);
	// right lower bushing taper
	translate([frame_length-frame_thickness-rail_inset, (frame_width-frame_thickness)/2, bush_height]) sphere(d=bush_dia);
	// right upper bushing
	translate([frame_length-frame_thickness-rail_inset, (frame_width-frame_thickness)/2, carriage_height-bush_height]) cylinder(d=bush_dia,h=bush_height+fuz);

	// belt clearance
	translate([(frame_length-frame_thickness-frame_slot_length)/2,frame_thickness,-fuz]) cube([frame_slot_length,carriage_width-2*frame_thickness+carriage_clearance,carriage_height+fuzz]);
	// belt tie off lower
	translate([(frame_length-frame_thickness+frame_slot_length)/2-rail_dia,carriage_width/2,1.5*rail_dia]) rotate([90,0,0]) cylinder(d=3.2, h=carriage_width+fuzz,center=true);
	// belt tie off upper
	translate([(frame_length-frame_thickness+frame_slot_length)/2-rail_dia,carriage_width/2,carriage_height-1.5*rail_dia]) rotate([90,0,0]) cylinder(d=3.2, h=carriage_width+fuzz,center=true);

    }
}

module assembly() {
    frame();
    translate([0,0,frame_height/2]) carriage();
    
    if (!integral_motor_bracket) {
	translate([0,frame_width,frame_height]) rotate([90,0,0]) color("orange") motor_bracket(integral_motor_bracket);
    }
    if (!integral_idler_bracket) {
	translate([0,frame_width-frame_thickness,0]) rotate([-90,0,0]) color("yellow") idler_bracket(length=frame_length, integral=false);
    }

}
if (part=="assembly") assembly();
if (part=="frame") frame();
if (part=="carriage") carriage();
if (part=="motor_bracket") motor_bracket(integral=integral_motor_bracket);
if (part=="idler_bracket") idler_bracket(integral=integral_idler_bracket);



    
