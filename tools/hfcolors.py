
from colormath.color_objects import LuvColor

Yn = 100.0
undash = 0.1978
vndash = 0.4684

saturated_udash = [0.4161, 0.1206, 0.1724, 0.3347, 0.2023, 0.1590, 0.2595, 0.1978]
saturated_vdash = [0.5285, 0.5613, 0.1681, 0.5119, 0.5204, 0.3052, 0.3079, 0.4684]

desaturated_udash = [0.3819, 0.1462, 0.1594, 0.2794, 0.2023, 0.1600, 0.2500, 0.1978]
desaturated_vdash = [0.5112, 0.5546, 0.2679, 0.4998, 0.5204, 0.3800, 0.3700, 0.4684]
colors = ['red','green','blue','orange','yellow','lightblue','pink','white']

def uv_to_rgb(udash, vdash, ltarget):
    Y = 0.0
    ll = 0.0
    while ((ll< ltarget) & (Y<100)):
        Yr = Y/Yn
        Y = Y+0.2
        if Yr<(6/29.0)**3 :
            Lstar = (29.0/3.0)**3*Yr
        else:
            Lstar = 116.0*Yr**(0.3333)-16.0
            ustar = 13*Lstar*(udash-undash)
            vstar = 13*Lstar*(vdash-vndash)
            color = LuvColor(Lstar, ustar, vstar)
            rgb = color.convert_to('rgb', target_rgb='sRGB')
            hsl = color.convert_to('hsl')
            ll = hsl.hsl_l
    return rgb

def sel_color(label, name, sat, lt):
    for i in range (0,8):
        if (colors[i] == name):
            if (sat):
                rgb = uv_to_rgb(saturated_udash[i],saturated_vdash[i], lt*0.01)
            else:
                rgb = uv_to_rgb(desaturated_udash[i],desaturated_vdash[i], lt*0.01)
            print '#define COLOR_%s Color(0x%02x, 0x%02x, 0x%02x) // %s%d lt %d #%02x%02x%02x' % (label, rgb.rgb_r, rgb.rgb_g, rgb.rgb_b, name, sat, lt, rgb.rgb_r, rgb.rgb_g, rgb.rgb_b)

def gen_table(i, lt):
    rgb = uv_to_rgb(saturated_udash[i],saturated_vdash[i], lt*0.01)
    print '%s_SATURATED_%02d #%02x%02x%02x' % (colors[i], lt, rgb.rgb_r, rgb.rgb_g, rgb.rgb_b)
    rgb = uv_to_rgb(desaturated_udash[i], desaturated_vdash[i], lt*0.01)
    print '%s_DESATURATED_%02d #%02x%02x%02x' % (colors[i], lt, rgb.rgb_r, rgb.rgb_g, rgb.rgb_b)

def snail():
    print 'snail'
#
    for c in range (0,10):
        x = 50.0+(30.0-50.0)*c/9
        rgb = uv_to_rgb(saturated_udash[3], saturated_vdash[3], x*0.01)
        print '{%d, 0x%02x, 0x%02x, 0x%02x},' % (c*10, rgb.rgb_r, rgb.rgb_g, rgb.rgb_b)
        #    print 'lift_%02d %02d #%02x%02x%02x' % (10-c, x, rgb.rgb_r, rgb.rgb_g, rgb.rgb_b)
        #    print 'lift_%02d %02d #%02x%02x%02x' % (10-c, x, rgb.rgb_r, rgb.rgb_g, rgb.rgb_b)

    rgb = uv_to_rgb(saturated_udash[7], saturated_vdash[7], 0.3)
    print '{%d, 0x%02x, 0x%02x, 0x%02x},' % (100, rgb.rgb_r, rgb.rgb_g, rgb.rgb_b)

    for c in range (0,10):
        x = 25.0+(40.0-15.0)*c/9
        rgb = uv_to_rgb(desaturated_udash[1], desaturated_vdash[1], x*0.01)
        print '{%d, 0x%02x, 0x%02x, 0x%02x},' % (110+c*10, rgb.rgb_r, rgb.rgb_g, rgb.rgb_b)
        #    print 'sink_%02d %02d #%02x%02x%02x' % (1+c, x, rgb.rgb_r, rgb.rgb_g, rgb.rgb_b)
        #    print 'sink_%02d %02d #%02x%02x%02x' % (1+c, x, rgb.rgb_r, rgb.rgb_g, rgb.rgb_b)

def bar():
    print 'bar'
    for c in range(0,8):
        gen_table(c, 10)
        gen_table(c, 15)
        gen_table(c, 20)
        gen_table(c, 25)
        gen_table(c, 30)
        gen_table(c, 35)
        gen_table(c, 40)
        gen_table(c, 45)
        gen_table(c, 50)

def define_cols():
    sel_color("alertSafe", "lightblue", 1, 55)
    sel_color("alertWarning", "orange", 1, 55)
    sel_color("alertAlarm", "red", 1, 45)
    sel_color("task", "pink", 0, 45)
    sel_color("bearing", "lightblue", 0, 20)
    sel_color("bearing_d", "lightblue", 0, 13)
    sel_color("wind", "lightblue", 0, 30)
    sel_color("wind_l", "lightblue", 0, 55)
    sel_color("fgabove", "green", 0, 30)
    sel_color("fgbelow", "red", 0, 40)
    sel_color("fgbelowlandable", "orange", 0, 60)
    sel_color("fgabove_d", "green", 0, 15)
    sel_color("fgbelow_d", "red", 0, 25)
    sel_color("fgbelowlandable_d", "orange", 0, 40)
    sel_color("ground", "orange", 0, 35)
    sel_color("ground_d", "orange", 0, 25)
    sel_color("sky", "lightblue", 0, 70)
    sel_color("sky_d", "lightblue", 0, 50)
    sel_color("landable_g", "green", 0, 35)
    sel_color("landable_m", "pink", 1, 60)
    sel_color("landable_o", "orange", 1, 45)
    sel_color("landable_r", "red", 1, 40)
    sel_color("landable_n", "white", 1, 60)

define_cols()

# mode_finalglide: water is sea color
# mode_abort: fgbelowlandable_d
# flarm_alarm: alertAlarm
# flarm_warning: alertWarning
# flarm_traffic: alertSafe
# map_small: task
# map_turnpoint: task
# map_taskturnpoint: task
# map_flag: sky
# map_thermal_source: sky

