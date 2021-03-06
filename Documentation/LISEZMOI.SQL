Version pour gpsdrive-1.31
Traduit en fran�ais par Jacky FRANCOIS <jfrancois@free.fr>

Depuis la version 1.29 GpsDrive supporte les bases de donn�es SQL.

Bases de donn�es support�es (October 2002): MySQL

Compiler:
---------
Le suport pour MySQL ne peut compiler que si vous avez installer
le package de d�velopement mysql-client et ne eut �tre utilis�
que si libmysqlclient.so est pr�sent.

Si vous ne voulez pas compiler le support de MySQL, m�me si les
librairies et les headers sont install�s, utilisez l'option 
"--disable-mysql" au moment du configure.

Ex�cution et initialisation:
----------------------------
Si ce n'est d�j� fait, installez le package du serveur mysql pour
votre distribution.

o D�marrez le serveur, habituellement en tapant: /etc/init.d/mysql start

o Pour cr�er une base de donn�es initiale utilisez le fichier "geoinfo.pl" 
  fourni. Assurez-vous que l'utilisateur choisi dispose des droits n�cessaires
  pour cr�er cette table.

  geoinfo.pl  --create-db --fill-defaults --openstreetmap \
              --db-user=root --db-password=<our mysql root password>
  (ajoutez un nom d'utilisateur si besoin est)

  Vous pouvez remplacer gast@localhost par gast@'%' si vous voulez utiliser
  la base de donn�es avec d'autres noms d'h�tes.


Mise � jour:
------------
Si vous avez d�j� la base de donn�es et que vous mettez GpsDrive � jour,
assurez-vous que la structure de votre table est bien comme celle du fichier
create.sql (old). Now use 
geoinfo.pl

Gestion du serveur SQL:
-----------------------

mysqlcc est un bon outil, t�l�chargez-le � cette adresse:

http://www.mysql.com/downloads/gui-mycc.html

S�curit�:
---------
GpsDrive acc�de par d�faut � la table "waypoints" dans la base de donn�es
"geoinfo" avec le nom d'utilisateur "gast" dont le mot de passe est "gast". 
Si vous voulez changer ces param�tres dans la base de donn�es, effectuez
�galement la modification dans GpsDrive. Pour cel�, �ditez le fichier
$HOME/.gpsdrive/gpsdriverc (dbuser,dbpass).

Gardez � l'esprit que le mot de passe pour cette base figure en clair dans
gpsdriverc!!!


Importer des fichiers de aypoints existants:
--------------------------------------------
Le script "wp2sql" permet de cr�er le fichier .sql � partir des fichiers
de waypoints de GpsDrive.

Adaptez-le � vos besoins!!!!

Tapez ensuite:

mysql -u gast -pgast <wp.sql

Vous pouvez �galement importe des fichies ASCII avec "mysqlimport".

=================================================================================
IMPORTANT:

Activez l'option 'Utiliser SQL' (dans le GUI) pour utiliser les waypoints de SQL.

=================================================================================

Cat�gories de waypoint:
-----------------------

Vous pouvez nommer vos cat�gories de waypoint comme vous l'entendez. Il existe
cependant des cat�gories pr�d�finies, auxquelles sont associ�s des icones dans
GpsDrive.

WLAN	     : point d'acc�s WLAN ouvert
WLAN-WEP     : point d'�cc�s WLAN crypt�
Rest	     : un restaurant
McDonalds    : un fast-food
BurgerKing   : un autre fast-food
Hotel	     : un hotel
Shop	     : magasin
Monu	     : Monument, attraction touristique
Speedtrap    : Radar de cont�le de vitesse, vous avertis quand vous allez passer 
               devant lui
Nightclub    : Pssst...
Airport	     : A�roport
Golf	     : Cours de golf
Gasstation   : Station essence
Cafe	     : Caf�

Les noms ne sont pas sensibles � la casse.

Sauvergarder et restaurer votre base de donn�e
----------------------------------------------

Sven a con�u gpssql_backup.sh et gpssql_restore.sh. Ces deux scripts sont simples
d'utilisation. Consultez simplement les commentaires dans ces scripts.
