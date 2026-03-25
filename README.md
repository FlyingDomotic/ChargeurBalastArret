# Control a ballast loader for model railroad (train stopped)/Contrôle d'un chargeur de ballast pour train miniature (version à l'arrêt)

[Cliquez ici pour la version française plus bas dans ce document](#france)

# <a id="english">English version</a>

This code controls a ballast loader into appropriate wagons.

It's based on a hopper driven by 2 pulse relays.

First opens hopper while second close it.

A rail has been equipped with 10 ILS regularly spaced, in order to detect a magnet placed under each wagon.

When a wagon goes over one of up to three opening ILS, a relay cuts power on rail, stopping train.After a settable time, a (settable) pulse is send to opening realy. After a (settable) time, hopper is closed, a power restored after a (settable) time.
    
This cycle can be repeated up to 3 times per wagon, for each wagon equipped with a magnet.

Note that circulation way of train has no impact if magnet is placed in middle of wagons.

It may happen that granulate block hopper exit. To balance this issue, you can add a vibrator that'll be activated when hopper opens until a (settable) time after hopper closing.

In addition, it's possible to add a supplemental ILS on unloading zone, that'll also activate vibrator during a settable time, to facilitate the descent of the ballast into the hopper.

On certain hopper models, it may happen that vibrations (re)open hopper. To counter balance this tendency, it's possible to define a re-closing delay, which will resend a closing pulse at regular interval as long as vibrations are active.

For more realistic experience, you can add a MP3 module that plays a sound during wagon loading, and another one during unloading. This module is not mandatory, don't add it if not useful. Still in the sale realistic spirit, sound can increase from 0 to its (settable) value over a (settable) time at loafing start, and decrease it from its maximum value to 0 at end of load.

Settings are send to Arduino through its serial (USB) link. This link is also used to send feedback and messages to user.

Settings are memorized in Arduino's EEPROM to be available after (re)start.

	
# Hardware Arduino Nano:
	- 11 ILS inputs (10 for wagon localization during load, 1 (optional) for unloading detection). Note that electronic magnetic captors can also be used.
	- 4 relays output:
        - to open hopper,
        - to close hopper,
        - for vibrator,
        - to cut power on rail.
    - 1 MP3 output:
        - to play loading and unloading sound, optional.

# Setting parameters:
    - find the proper pulse duration to open and close hopper, without burning coils, increasing or reducing `ID` parameter and using `O` (open) and `C` (close) hopper,
 	- find loading ILS number 1 manually pushing wagon, set it in `IL1` parameter,
    - repeat previous step up to 3 times, if needed,
    - load `WS` with delay to wait after stopping train, before loading wagon,
    - find wagon filling duration and set it into `FD`,
    - load `WF` with delay to wait after filling wagon, before starting train,
    - if vibrations are needed:
        - vibrator is activated when hopper opens, stopper after `FV` delay when closed,
        - vibrator is activated when unloading ILS is activated, for `UV` delay,
        - set hopper re-closing interval into `RC`,
    - if sound id needed:
        - set sound volume into `SV`, testing sounds with `TS`,
        - set sound increment into `SI`, if needed,
        - set loading sound index into `LS`, unload sound into `US`
    - start process with `R`,
    - send train over ILS.

# Commands:
    - I11-10 : ILS 1 (number)
    - I20-10 : ILS 2 (number)
    - I30-10 : ILS 3 (number)
    - ID1-999 : Relay Impulsion Duration (ms)
    - FD1-9999 : Wagon Filling DuraTion (ms)
    - FV0-9999 : Filling Vibrations (ms)
    - UV0-9999 : Unloading Vibrations (ms)
    - RC0-9999 : Repeat Close (ms)
    - WS0-9999 : Wait After Stop (ms)
    - WF0-9999 : Wait After Filling (ms)
    - LS0-99 : Loading sound
    - US0-99 : Unloading Sound
    - TS1-99 : Test sound
    - SV0-30 : Sound Volume
    - SI0-999 : Sound Increment (ms)
    - R : Run
    - S : Stop
    - E : Emergency Stop
    - IS : ILS State
    - O : Open Hopper
    - C : Close Hopper
    - ST : Stop Train
    - RT : Run Train
    - D : Toggle Debug
    - INIT : Global Initialization
    - DV : Display Variables

## Pushing <ESC> (sometimes wtriing Escape):
    - Emergency stop:
        - Stop process,
        - Close hopper,
        - Stop vibrations,
        - Stop train,
        - Stop sound.

Author : Flying Domotic, February 2025, for FabLab
License: GNU GENERAL PUBLIC LICENSE - Version 3, 29 June 2007

# <a id="france">Version française</a>

Ce code contrôle un chargeur de ballast dans des wagons adéquats.

Il est basé sur une trémie commandée par 2 relais à impulsion.

Le premier relais ouvre la trémie alors que le second la ferme.

Un rail a été équipé de 10 ILS régulièrement espacés, afin de détecter l'aimant placé sous chaque wagon.

Lorsqu'un wagon passe au dessus de (l'un des jusqu'à trois) ILS d'ouverture (paramétrable), un relais coupe l'alimentation de la voie, stoppant le train. Après un temps (réglable), une impulsion (de durée réglable) est envoyée au relais d'ouverture. La trémie est fermée après un temps paramétrable, puis, après une attente réglable, l'aliemntation est remise et le train avance.
    
Ce cycle peut être répété jusqu'à 3 fois par wagon, pour chaque wagon équipé d'un aimant.

Noter que le sens de circulation des trains n'a pas d'impact si l'aimant est placé au milieu des wagons.

Il arrive que le granulat bloque la sortie de la trémie. Pour contrer ce problème, on peut ajouter un vibreur qui sera alimenté lorsque la trémie s'ouvre jusqu'à expiration d'un temps donné après la fermeture de la trémie.

De plus, il est possible d'ajouter un ILS supplémentaire sur la zone de déchargement, qui activera également le vibreur pendant un temps donné, afin de faciliter la descente du ballast dans la trémie.

Sur certains modèles de trémie, il arrive que ces vibrations (ré)ouvrent la trémie. Pour contrer cette tendance, il est possible de définir un délai de refermeture de la trémie, qui renverra une impulsion de fermeture à intervalle régulier tant que les vibrations seront actives.

Pour plus de réalisme, on peut ajouter un module MP3 qui permet de jouer un son pendant le chargement des wagons et un autre pendant le déchargement. Ce module n'est pas obligatoire, ne pas l'installer si inutile. Toujours dans le même souci de réalisme, le son augmente de 0 à sa valeur maximale sur une dure fixée au début du chargement, et réduit de sa valeur maximale à 0 sur la même durée à la fin du chargement.

Les réglages sont envoyés à l'Arduino au travers de sa liaison série. Ce même moyen est utilisé pour envoyer les messages à l'utilisateur.

Les réglages sont mémorisés dans l'EEPROM de l'Arduino afin d'être disponibles après son redémarrage.
	
# Hardware Arduino Nano:
	- 11 entrées ILS (10 pour l'avancement du wagon lors du chargement, 1 pour la détection du déchargement)
	- 4 sorties relais :
        - pour l'ouverture de la trémie,
        - pour sa fermeture,
        - pour le vibreur,
        - pour la coupure de l'alimentation de la voie.
    - 1 sortie son MP3:
        - pour jouer les sons de chargement et déchargement, optionnel.

# Paramétrage :
    - on cherche la durée d'impulsion nécessaire pour ouvrir ou fermer la trémie à coup sur, sans faire chauffer les bobines, en augmentant/réduisant la valeur du paramètre `DI` et utilisant les commandes `O` et `F` pour ouvrir/fermer la trémie
	- on repère le numéro de l'ILS du remplissage 1 en avançant le wagon à la main, on le charge dans `IL1`,
    - on répète l'opération jusqu'à 3 fois si besoin,
    - charger `AA` avec le délai à attendre après avoir stoppé le train, avant de charger le wagon,
    - rechercher la durée de remplissage du wagon et le mettre dans `DR`,
    - charger `AR` avec le délai à attendre après avoir rempli le wagon, avant de redémarrer le train,
    - si on souhaite utiliser les vibrations :
        - on active le vibreur à l'ouverture de la trémie, on l'arrête après le délai `VV` à la fermeture,
        - on active le vibreur au passage sur l'ILS de vidage, on l'arrête après le délai `VV`,
        - on définit le délai refermeture dans `RF` si besoin.
    - si on souhaite utiliser le son :
        - mettre le volume dans `V`, tester les sons avec `TS`,
        - mettre l'incrément de son dans `IS`, si besoin,
        - mettre le son de chargement dans `SC`, de déchargement dans `SD`.
    - lancer le processus avec `M`,
    - envoyer le train sur les ILS.

# Commandes :
    - I11-10 : ILS 1 (numéro)
    - I20-10 : ILS 2 (numéro)
    - I30-10 : ILS 3 (numéro)
    - DI1-999 : Durée Impulsion relais (ms)
    - DR1-9999 : Durée Remplissage wagon (ms)
    - VR0-9999 : Vibrations Remplissage (ms)
    - VV0-9999 : Vibrations Vidage (ms)
    - RF0-9999 : Répétition fermeture (ms)
    - AA0-9999 : Attente après Arrêt (ms)
    - AR0-9999 : Attente après remplissage (ms)
    - SC0-99 : Numéro du son de chargement (0 si inutile)
    - SD0-99 : Numéro du son de déchargement (0 si inutile)
    - TS1-99 : Numéro du son à jouer en test
    - V0-30 : volume du son (0 si inutile)
    - IS0-999 : durée d'incrément du son (ms)
    - M : Marche (détection passage wagon activée)
    - A : Arrêt (stoppe la détection des wagons)
    - E : Etat ILS
    - U : arrêt d'urgence (ferme la trémie, arrêt du processus, des vibrations, du train et du son)
    - O : Ouverture trémie
    - F : Fermeture trémie
    - AT : Arrêt Train
    - DT : Démarrage Train
    - D : Bascule déverminage
    - AV : Afficher variables
    - INIT : Initialisation globale

## Appui sur <ESC> (parfois marquée Escape ou Echap.) :
    - Arrêt d'urgence :
        - Arrêt du processus,
        - Ferme la trémie,
        - Arrête les vibrations,
        - Arrête le train,
        - Stoppe le son.

Auteur : Flying Domotic, Février 2025, pour le FabLab
Licence: GNU GENERAL PUBLIC LICENSE - Version 3, 29 June 2007
