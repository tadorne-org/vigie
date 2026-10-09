# vigie.tadorne.org — site produit

Site Hugo de **Vigie**, la carte intégrée du nœud énergie, présentée comme un
projet **Tadorne**. Bilingue EN (racine) / FR (`/fr/`), sans thème externe,
sans JavaScript, sans requête tierce.

Structure et système de marque repris du site de Penon ([tadorne-org/penon](https://github.com/tadorne-org/penon)).

## Construire

```console
$ hugo server          # http://localhost:1313/  (FR sur /fr/)
$ hugo --minify        # build de production dans public/
```

Requiert Hugo ≥ 0.146 (testé avec 0.165.0 extended). `public/` n'est pas
versionné.

## Structure

```
hugo.toml              config, langues, menus, params (dont seoIndexable)
i18n/{en,fr}.toml      chaînes d'interface
content/en/            contenu anglais  → /
content/fr/            contenu français → /fr/
assets/css/            tokens.css (marque Tadorne) + fonts.css + main.css
static/fonts/          5 woff2 auto-hébergés
static/img/            visuels du projet (convertis depuis les sources du dépôt)
layouts/               home, page, roadmap, log/, _partials/, _markup/
```

## Publier une info (le journal)

Les deux fichiers doivent porter le **même `translationKey`** — c'est le seul
lien entre les langues :

```console
$ hugo new content/en/log/ma-note.md
$ hugo new content/fr/journal/ma-note.md
```

```yaml
---
title: "Titre"
translationKey: log/ma-note
date: 2026-09-15
description: "Une ou deux phrases : listes, RSS et og:description."
params:
  tags: ["vigie"]
---
```

Le fil RSS est sur `/log/index.xml` et `/fr/journal/index.xml`.

Une note publiée apparaît aussi dans la liste « Source : le journal » de la page
Documentation : le shortcode `log-sources` relit la section du journal à chaque
build, sans qu'il y ait à toucher la page.

## Sources des gabarits

Le site reprend à l'identique la structure, la feuille de style et les partiels
du site de Penon. Les adaptations propres à Vigie sont :

- le titre, le `baseURL` (`https://vigie.tadorne.org/`), les descriptions ;
- le mot de marque dans l'en-tête (`vigie`) et le nom du bundle CSS
  (`css/vigie.css`, dans `layouts/_partials/head.html`) ;
- la page « Conception » (`/design/`, `/fr/conception/`) qui remplace la page
  « Financement » ;
- la page « Documentation » (`/documentation/`, `/fr/documentation/`), la
  documentation technique illustrée de Vigie 1.0 (architecture, carte, mesure,
  micrologiciel, réseau), avec un disclaimer « travail en cours » en tête et la
  liste des notes générée par le shortcode `log-sources` ;
- les figures de la page, dans `static/img/doc/` : rendus et plans de la carte
  (`pcb/build/`), montage au bus négatif (figure d'étude), captures de l'app
  Tadorne (FR et EN), et un schéma-bloc de la carte généré en SVG
  (`vigie-blocs-fr.svg`, `vigie-blocs-en.svg`). Le dossier garde aussi les
  figures d'étude que la page n'utilise pas encore (synoptique, hivernage,
  coffret, support de panneau), converties depuis les sources du dépôt ;
- le contenu, rédigé à partir de `docs/`, `pcb/README.md`, `firmware/README.md`
  et `ios/README.md` du dépôt.

Les figures d'étude datent de l'empilage de modules (INA228, LilyGo) et sont
légendées comme telles ; la documentation décrit la carte intégrée.

## Indexation : ouverte

`params.seoIndexable = true` dans `hugo.toml` → aucune balise `robots` sur les
pages, `robots.txt` en `Allow: /` et sitemap annoncé.

Le site était en divulgation restreinte jusqu'à la mise au banc de la carte.
Repasser le paramètre à `false` referme tout d'un coup : `noindex, nofollow,
noarchive` sur chaque page et `Disallow: /`.

## Déploiement

`hugo --minify` produit un `public/` statique — n'importe quel hébergeur
statique convient. `netlify.toml` est fourni.

`baseURL` vaut `https://vigie.tadorne.org/` ; le surcharger au besoin avec
`hugo --baseURL https://exemple/`.

## Licences

- Contenu (`content/`, images, vidéos, modèles 3D) : [CC BY-SA 4.0](LICENSE-CONTENT).
- Gabarits, CSS, JavaScript et configuration : [MIT](LICENSE).
- Polices (`static/fonts/`) : SIL Open Font License 1.1, propriété de leurs auteurs.
- `static/files/` : fichiers de fabrication publiés avec leur propre licence
  (CERN-OHL-S v2, texte joint dans chaque dossier).
