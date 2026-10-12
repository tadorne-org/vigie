// ============================================================
//  NŒUD ÉNERGIE — First 211 (Vigie)
//  SEMELLE TPU 95A, PLEIN FOND, pour le coffret étanche du commerce
//
//  Une seule pièce souple qui tapisse tout le fond du coffret et porte :
//    - le SHUNT 100 A / 75 mV, dans un berceau qui l'enveloppe et
//      DÉBORDE par-dessus ses blocs de `debord` mm. On l'y enfonce par
//      le dessus : les lèvres s'écartent (élasticité du TPU) puis se
//      referment. La poche est en plus plus courte que le shunt
//      (`serrage`), il est pincé en longueur.
//    - la CARTE VIGIE 86 x 54, sur 4 colonnettes, vis M3.
//  Posée au fond, la semelle touche les quatre parois et les colonnes
//  d'angle : elle ne bouge pas, le berceau ne peut plus s'ouvrir, le
//  shunt est prisonnier. Deux vis M4 dans l'axe la plaquent au fond.
//  Dessous plat, sans aucun logement : les picots
//  moulés du fond sont à couper au ras, les bossages restent dessous.
//
//  Sous chaque bloc du shunt dépasse une tête de boulon M8 (13,5 mm) :
//  un logement est creusé dessous. Sous la barre de manganin (7,5 W à
//  100 A), rien ne touche : le TPU ramollit vers 80 °C.
//
//  DURCISSEMENT (chocs / vibrations, esprit MIL-STD-810) :
//    - TAMPON sous le couvercle (part = "tampon") : appuie sur le sommet
//      du berceau avec `ecrasement` mm de précontrainte. Le shunt est
//      serré entre le fond et le couvercle, zéro jeu, les lèvres ne
//      peuvent plus s'ouvrir sous un choc vertical.
//    - Lèvres de 3 mm. Pas de congé sous la lèvre : il empêcherait le
//      bloc (arêtes vives) de s'asseoir ; la précontrainte du tampon
//      reprend l'effort à la place.
//    - Abouts du berceau échancrés jusqu'au dessus des blocs : la cosse
//      M8 sort à plat. Devant chaque about, un ANCRAGE RILSAN tient le
//      câble 16 mm² : sa masse ne fait plus levier sur le boulon.
//    - BAGUES DE COMPRESSION dans les trous M4 (entretoise inox lisse
//      Ø6 / Ø4,2, ou tube laiton 6 x 0,8) : la vis serre métal sur
//      bossage, le TPU (qui flue) reste précontraint de `precontrainte`.
//      Vis inox A4, écrou Nylstop ou frein-filet, trait de peinture témoin.
//
//  Repère : origine au CENTRE du fond intérieur du coffret, Z = 0 sur le
//  fond. X = largeur (shunt contre la paroi -X), Y = longueur (+Y = haut
//  de la photo).
//
//  Impression : TPU 95A, à plat, sans support (lèvres de 3 mm en
//  porte-à-faux, pont court). 3 périmètres, 25 % gyroïde dans la dalle :
//  le remplissage fait office d'amortisseur. Berceau, lèvres et ancrages
//  imprimés PLEINS (modificateur 100 % dans le trancheur, Z > ep_base). 20–25 mm/s, rétraction courte.
// ============================================================

part = "tout";     // "semelle" | "tampon" | "modificateur" | "tout" (assemblage)
$fn  = 40;

/* ============ 1. COFFRET (plan moulé au fond) ============ */
int_L   = 151;     // longueur intérieure AU FOND                     >>> A MESURER
int_W   = 83.2;    // largeur intérieure AU FOND                      >>> A MESURER
util_L  = 139.7;   // longueur libre entre les colonnes d'angle       >>> A MESURER
col_x   = 12;      // emprise des colonnes d'angle selon X            >>> A MESURER
jeu     = 0.4;     // jeu contre les parois, par bord

// Bossages et picots moulés : la semelle a un dessous plat. Les picots
// sont à couper à la pince au ras du fond ; les bossages « papillon »
// (≈ 3,5 mm) sont écrasés par le TPU ou arasés au cutter.

// Deux trous de fixation dans l'axe, au droit des trous haut et bas du
// fond : vis M4 + rondelle large, la semelle est plaquée au fond.
fix_y = [ 139.7/2, -139.7/2 ];   // >>> ENTRAXE A VERIFIER
fix_d = 4.5;                     // passage M4

/* ============ 2. SHUNT 100 A / 75 mV, FL-2 ============ */
shunt_L      = 103;   // hors-tout, bloc à bloc                     >>> A RECONFIRMER
shunt_W      = 22;    // largeur des blocs
shunt_bloc_H = 8;     // épaisseur d'un bloc
shunt_bloc_L = 30;    // longueur d'un bloc
shunt_barre_ep = 1.8;
shunt_H_tot  = 25;    // du dessous du bloc au bout du goujon M8
// Tête de boulon M8 sous chaque bloc
boulon_s   = 13.5;    // cote relevée sur la tête
boulon_h   = 5.5;     // hauteur de tête                            >>> A MESURER
boulon_e   = 86;      // entraxe des deux boulons M8 (mesuré)
boulon_d   = boulon_s / cos(30) + 0.8;   // logement rond : passe la tête
                                          // quelle que soit son orientation

/* ============ 3. CARTE VIGIE (pcb/scripts/place_board.py) ============ */
pcb_L = 86;  pcb_W = 54;  ep_pcb = 1.6;
pcb_trous = [ [4, 4], [75.5, 4], [4, 50], [75.5, 50] ];  // repère carte
// Carte posée en long : bord x = 0 de la carte (borniers J1/J2, entrée
// des fils) vers le HAUT (+Y), bord y = 0 côté shunt, USB-C côté paroi.
pcb_y_haut = 43;      // Y du bord x = 0 de la carte
pcb_libre  = 5;       // garde sous la carte, au-dessus de la semelle
col_pcb_d  = 9;       // Ø des colonnettes
vis_pcb_d  = 2.6;     // avant-trou pour vis M3 autotaraudeuse plastique
vis_pcb_p  = 9;       // profondeur de l'avant-trou

/* ============ 4. SEMELLE ============ */
ep_base   = 6;        // épaisseur de la semelle hors berceau
serrage   = 0.6;      // poche plus COURTE que le shunt
serrage_w = 0.2;      // poche plus ÉTROITE que le shunt
mur_ext   = 2.5;      // paroi du berceau côté coffret
mur_int   = 3;        // paroi du berceau côté carte
debord    = 3;        // les lèvres recouvrent le dessus des blocs de 3 mm
levre_ep  = 3;        // épaisseur des lèvres
cosse_w   = 20;       // échancrure des abouts pour la cosse M8

// Ancrages rilsan des câbles 16 mm², collés aux abouts du berceau
anc_w     = 16;       // largeur selon X
anc_l     = 11;       // longueur selon Y, depuis l'about
rils_w    = 5.5;      // tunnel pour collier de 4,8 mm
rils_h    = 2.5;
rils_ep   = 2.5;      // matière au-dessus du tunnel

// Bagues de compression des vis M4
bague_d   = 5.9;      // logement, serrage sur une bague Ø6
precontrainte = 0.3;  // la bague est plus courte que la dalle de 0,3 mm

// Tampon sous le couvercle
prof_int   = 55;      // fond -> dessous du couvercle fermé       >>> A MESURER
ecrasement = 1.5;     // précontrainte du tampon sur le berceau
tampon_ame = 4;       // âme du tampon, collée au couvercle
kel_y      = shunt_L/2 - 24;   // vis Kelvin depuis le centre     >>> A MESURER

sx0  = -int_W/2 + jeu;
pk_x0 = sx0 + mur_ext;
pk_w  = shunt_W - serrage_w;
pk_L  = shunt_L - serrage;
sx1   = pk_x0 + pk_w + mur_int;           // bord du berceau côté carte
bx    = pk_x0 + pk_w/2;                   // axe du shunt
ber_L = pk_L + 2*mur_int;


// Dessous des blocs : logement des têtes de boulon + 2 mm de TPU
lift   = boulon_h + 0.5 + 2;
h_ber  = lift + shunt_bloc_H + levre_ep;  // sommet du berceau

pcb_z  = ep_base + pcb_libre;            // dessous de la carte, depuis le fond
pcb_x0 = sx1 + 0.5;                       // bord de carte côté shunt
pcb_pos = [ for (t = pcb_trous) [pcb_x0 + t[1], pcb_y_haut - t[0]] ];

echo(str("Coussin sous les blocs ", lift, " mm ; berceau ", h_ber,
         " mm ; bout des goujons à ", lift + shunt_H_tot, " mm du fond"));
echo(str("Carte : X ", pcb_x0, " .. ", pcb_x0 + pcb_W, " (paroi à ",
         int_W/2, ") ; Y ", pcb_y_haut - pcb_L, " .. ", pcb_y_haut));
echo(str("Ouverture entre lèvres : ", pk_w - 2*debord, " mm"));
echo(str("Bague de compression : Ø6 / Ø4,2 x ", ep_base - precontrainte, " mm"));
echo(str("Tampon : ", prof_int - h_ber + ecrasement, " mm de haut, ",
         ecrasement, " mm d'écrasement"));
assert(prof_int - tampon_ame > lift + shunt_H_tot + 3,
       "coffret trop peu profond : l'âme du tampon toucherait les goujons M8");

/* ============ 5. PIÈCE ============ */
module dalle(h) {
  // fond complet moins les colonnes d'angle, arêtes vives
  difference() {
    translate([-int_W/2 + jeu, -int_L/2 + jeu, 0])
      cube([int_W - 2*jeu, int_L - 2*jeu, h]);
    for (sx = [-1, 1], sy = [-1, 1])
      translate([sx*(int_W/2 - col_x/2), sy*(int_L/2), 0])
        cube([col_x + 2*jeu, int_L - util_L + 2*jeu, 3*h], center = true);
  }
}

module levre(l) {
  // lèvre en porte-à-faux vers l'intérieur, section rectangulaire ;
  // part de 0,5 mm dans la paroi pour fusionner proprement
  translate([-0.5, 0, 0]) cube([debord + 0.5, l, levre_ep - 0.05]);
}

module semelle() {
  union() {
  difference() {
    union() {
      dalle(ep_base);
      // berceau
      translate([sx0, -ber_L/2, 0]) cube([sx1 - sx0, ber_L, h_ber]);
      // ancrages rilsan, dans l'axe du shunt, contre chaque about
      for (s = [1, -1]) translate([bx - anc_w/2, s > 0 ? ber_L/2 - 0.5 : -ber_L/2 - anc_l, 0])
        cube([anc_w, anc_l + 0.5, lift + shunt_bloc_H]);
      // colonnettes de la carte
      for (p = pcb_pos) translate([p[0], p[1], 0]) cylinder(d = col_pcb_d, h = pcb_z);
    }
    // poche du shunt (jusqu'au sommet, les lèvres sont rajoutées après)
    translate([pk_x0, -pk_L/2, lift]) cube([pk_w, pk_L, 50]);
    // sous la barre de manganin : vide jusqu'à la dalle
    translate([pk_x0, -(pk_L/2 - shunt_bloc_L), ep_base])
      cube([pk_w, pk_L - 2*shunt_bloc_L, 50]);
    // logements des têtes de boulon
    for (s = [1, -1]) translate([bx, s*boulon_e/2, lift - boulon_h - 0.5])
      cylinder(d = boulon_d, h = boulon_h + 1);
    // abouts échancrés jusqu'au dessus des blocs : la cosse sort à plat
    for (s = [1, -1]) translate([bx - cosse_w/2, s > 0 ? pk_L/2 - 1 : -ber_L/2 - 1, lift + shunt_bloc_H])
      cube([cosse_w, mur_int + 2, 50]);
    // tunnels des colliers rilsan (selon X, sous le dessus de l'ancrage)
    for (s = [1, -1])
      translate([bx - anc_w/2 - 1, s*(ber_L/2 + anc_l/2) - rils_w/2,
                 lift + shunt_bloc_H - rils_ep - rils_h])
        cube([anc_w + 2, rils_w, rils_h]);
    // logements des bagues de compression M4
    for (y = fix_y) translate([0, y, 0]) cylinder(d = bague_d, h = 50, center = true);
    // avant-trous des vis de la carte
    for (p = pcb_pos) translate([p[0], p[1], pcb_z - vis_pcb_p]) cylinder(d = vis_pcb_d, h = 50);
  }
  // lèvres sur les deux côtés, toute la longueur (les abouts sont
  // échancrés pour la cosse, le tampon reprend l'effort vertical)
  zl = lift + shunt_bloc_H;
  translate([pk_x0, -pk_L/2 + 1.5, zl]) levre(pk_L - 3);
  translate([pk_x0 + pk_w, -pk_L/2 + 1.5, zl]) mirror([1, 0, 0]) levre(pk_L - 3);
  }
}

// Tampon collé sous le couvercle (VHB ou silicone), en TPU 95A plein.
// U renversé : deux longerons au-dessus des parois du berceau (lèvres
// comprises), reliés en haut par une âme pleine de `tampon_ame` mm.
// Le U est bien plus raide que deux lames libres, qui flamberaient. Les cosses et
// les écrous M8 passent entre les longerons ; le longeron côté carte
// est échancré au droit des vis Kelvin pour laisser passer les fils.
// Repère d'assemblage ; part = "tampon" le retourne pour l'impression.
module tampon() {
  z0 = h_ber - ecrasement;  z1 = prof_int;
  difference() {
    union() {
      translate([sx0, -ber_L/2, z0]) cube([pk_x0 + debord - sx0, ber_L, z1 - z0]);
      translate([pk_x0 + pk_w - debord, -ber_L/2, z0]) cube([sx1 - (pk_x0 + pk_w - debord), ber_L, z1 - z0]);
      translate([sx0, -ber_L/2, z1 - tampon_ame]) cube([sx1 - sx0, ber_L, tampon_ame]);
    }
    for (s = [1, -1]) translate([pk_x0 + pk_w - debord - 1, s*kel_y - 5, z0 - 1])
      cube([sx1 - pk_x0 - pk_w + debord + 2, 10, 7]);
  }
}

/* ============ 6. FANTÔMES ============ */
module fantome_shunt() {
  color("silver", 0.85) translate([bx, 0, lift]) {
    for (s = [1, -1]) translate([0, s*(shunt_L/2 - shunt_bloc_L/2), 0])
      translate([-shunt_W/2, -shunt_bloc_L/2, 0]) cube([shunt_W, shunt_bloc_L, shunt_bloc_H]);
    for (s = [1, -1]) translate([0, s*boulon_e/2, 0]) {
      translate([0, 0, -boulon_h]) cylinder(d = boulon_s / cos(30), h = boulon_h, $fn = 6);
      cylinder(d = 8, h = shunt_H_tot);
      translate([0, 0, shunt_bloc_H]) cylinder(d = 13 / cos(30), h = 6.5, $fn = 6);
    }
    for (dx = [-5, 0, 5])
      translate([dx - 1, -shunt_L/2 + shunt_bloc_L, (shunt_bloc_H - shunt_barre_ep)/2])
        cube([2, shunt_L - 2*shunt_bloc_L, shunt_barre_ep]);
  }
}

module fantome_coffret() {
  color("dimgray", 0.25) translate([-int_W/2 - 3, -int_L/2 - 3, -3]) cube([int_W + 6, int_L + 6, 3]);
  color("dimgray", 0.4) {
    for (sx = [-1, 1], sy = [-1, 1])
      translate([sx > 0 ? int_W/2 - col_x : -int_W/2, sy > 0 ? util_L/2 : -int_L/2, 0])
        cube([col_x, (int_L - util_L)/2, 40]);
  }
}

module fantome_pcb() {
  color("darkgreen", 0.8) difference() {
    translate([pcb_x0, pcb_y_haut - pcb_L, pcb_z]) cube([pcb_W, pcb_L, ep_pcb]);
    for (p = pcb_pos) translate([p[0], p[1], 0]) cylinder(d = 3.2, h = 50);
  }
}

// Volume modificateur pour Cura : berceau et ancrages au-dessus de la
// dalle, imprimés PLEINS (« Modifier les réglages des chevauchements »).
// Cura centre chaque STL sur son encombrement : quatre micro-cubes posés
// dans les coins (vides, découpés par les colonnes d'angle) donnent au
// modificateur EXACTEMENT l'encombrement de la semelle. Placés tous deux
// en X = 0, Y = 0, ils se superposent.
module modificateur() {
  translate([sx0, -ber_L/2 - anc_l, ep_base])
    cube([sx1 - sx0, ber_L + 2*anc_l, h_ber - ep_base]);
  for (sx = [-1, 1], sy = [-1, 1])
    translate([sx > 0 ? int_W/2 - jeu - 0.2 : -int_W/2 + jeu,
               sy > 0 ? int_L/2 - jeu - 0.2 : -int_L/2 + jeu, 0])
      cube([0.2, 0.2, sy > 0 && sx > 0 ? h_ber : 0.2]);
}

if (part == "semelle") semelle();
else if (part == "modificateur") modificateur();
else if (part == "tampon") translate([0, 0, prof_int]) mirror([0, 0, 1]) tampon();
else {
  color("orange") semelle();
  color("tomato", 0.6) tampon();
  fantome_shunt();
  fantome_pcb();
  fantome_coffret();
}
