# Contrôle d'un chargeur de ballast pour train miniature (version à l'arrêt)

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

Pour plus de réalisme, on peut ajouter un module MP3 qui permet de jouer un son pendant le chargement des wagons et un autre pendant le déchargement. Ce module n'est pas obligatoire, ne pas l'installer si inutile. Toujours dans le même souci de réalisme, le son augmente de 0 à sa valeur maximale sur une dure fixée au début du chargement, et réduit de sa valeur maximale à 0 sur la même durée lors du déchargement.

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
    - on cherche la durée d'impulsion nécessaire pour ouvrir ou fermer la trémie à coup sur, sans faire chauffer les bobines, en augmentant/réduisant la valeur du paramètre DIR et utilisant les commandes O et F pour ouvrir/fermer la trémie
	- on repère le numéro de l'ILS du remplissage 1 en avançant le wagon à la main
    - on répète l'opération jusqu'à 3 fois si besoin
    - si on souhaite utiliser les vibrations :
        - on active le vibreur à l'ouverture de la trémie, on l'arrête ferme après un délai à la fermeture
        - on active le vibreur au passage sur l'ILS de vidage, on l'arrête après un autre délai à la fermeture

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
        - Arrêt du processus
        - Ferme la trémie
        - Arrête les vibrations
        - Arrête le train
        - Stoppe le son

Auteur : Flying Domotic, Février 2025, pour le FabLab
Licence: GNU GENERAL PUBLIC LICENSE - Version 3, 29 June 2007
