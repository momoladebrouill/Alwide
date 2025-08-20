<toc/>

<center>
# Rapport de stage
</center>

## 1. Introduction

```c
int test;

void main(int argc, char**args){


	for(int i = 0 ; i < 100 ; i++){

		printf("Hello World !");

	}

	return 0;
}
```

Dans le cadre de ma formation à l'Ensimag, j'ai eu à réaliser un stage en entreprise pour découvrir le métier
d'ingénieur.

Je suis un étudiant en 1ᵉ année qui intègrera en septembre 2025 le MOSIG-M1 en double dîplome avec l'Ensimag.
Ce cursus me permet de m'orienter vers de
l'informatique théorique (algorithmie, théorie des langages, IA) et de la programmation bas niveaux (systèmes
d'exploitation, design des compilateurs).

C'est ce qui m'a motivé pour passer le stage assistant ingénieur en 1ᵉ année.
Cela me permet de libérer l'été prochain pour potentiellement faire un stage à l'étranger. Ce stage à l'étranger me
permettrait de valider ma compétence *travailler à l'étranger* et ainsi pouvoir faire le semestre 8 dans le master en
M2 (plutôt qu'en échange).

Etant orignaire du Cantal, j'ai donc cherché les entreprises d'informatique dans le département. C'est là que
j'ai découvert AGEDI. \
Mais alors qu'est-ce qu'est AGEDI ?

## 2. Présentation d'AGEDI et contexte du stage

AGEDI est un _"Établissement Public Administratif sous forme de Syndicat Mixte Ouvert"_ d'un effectif de 64 salariés.
Son objectif est de développer des suites logicielles à destination des petites et moyennes collectivités
territoriales. Une collectivité territoriale représente généralement une commune, ou une agglomération de commune.
Le but d'AGEDI est donc d'offrir une solution logicielle informatique peu chère et complète pour faciliter la
gestion leurs services et ressources.

AGEDI ne développe pas uniquement les logiciels mais propose aussi tout un environnement de suivis et d'accompagnement pour les adhérents :

- L'**équipe de developpement** est composé de 28 salariés qui corrigent, maintiennent et font évoluer les logiciels.
- Le **service support adhérent** est composé de 15 salariés qui traitent les questions et problèmes rencontrés par les utilisateurs,
  les bugs et les disfonctionnements sont remontés sous forme de ticket à l'équipe de développement.
- L'**équipe de formateurs** est elle composé de 13 salariés qui accompagnent et facilitent la migration des collectivités sur les logiciels 
  Proxima.

Maintenant parlons chiffres : plus de **6000** collectivités sont actuellement adhérentes à au moins un module de Proxima. Ces collectivités peuvent être
responsables de **10** à **30 000** habitants.\
Le défi est donc de pouvoir proposer des logiciels peu chers aux collectivités avec peu de moyens, mais également de
fournir des logiciels complet et puissant pour répondre aux besoins des plus grosses.

Ensuite un peu de contexte sur les gammes de logiciels. AGEDI existe depuis près de 28 ans et a proposé
plusieurs gammes de logiciels suivant l'évolution des usages et des technologies. Il y a quelques années
une nouvelle gamme a été créé : la gamme _**Proxima**_.

Cette gamme vise à remplacer les **clients lourds**, par des **clients légers** utilisables depuis un navigateur web. \
Un client lourd est un logiciel dont le fonctionnement et les données sont installées directement sur la machine de
l'utilisateur, à l'inverse un client léger est un logiciel accessible à partir d'une connexion internet et dont les
données sont stockées sur un serveur distant.

La gamme Promixa contient **12** logiciels (ou *modules*) visant des domaines très variés. Pour n'en citer que quelques-uns :

- **Proxima.FIN** : logiciel de gestion comptable et financière
- **Proxima.ACTE** : logiciel de gestion des actes et transmission au contrôle de légalité
- **Proxima.CIM** : logiciel de gestion des cimetières communaux
- **Proxima.CITE** : logiciel de gestion simplifié des listes électorales et du recensement citoyen (JDC), module
  d'annuaire et base documentaire
- **Proxima.EAU** : logiciel de gestion des compteurs d'eaux et assainissement

<p style="margin-bottom:0;">Un point important à noter est le rôle de l'équipe de développement :</p>
<center style="margin:0">
*"corrigent, maintiennent et font évoluer les logiciels"*
<center/>

<p style="margin-top:0;">
En effet la base des logiciels n'est pas développé directement par AGEDI, le socle du code est sous-traité
a des entreprises spécialisées dans le développement rapide et à faible coût de logiciels. 
Une fois la base de code reçu le logiciel est intégré à l'infrastructure de Proxima et subit de nombreux
ajustements. Une fois en production le logiciel est susceptible de recevoir un certains nombre de correctifs et d'évolutions
basé sur le retour d'utilisation des adhérents.\
Le suivis des logiciels est divisé en **5** unités de 3 à 7 personnes, qui sont chargés du suivis de 3 à 4 modules.

<p/>


<p style="margin-bottom:0;">Dans mon cas j'ai rejoins l'unité **Facturation / Cimetière** qui gère les modules suivants : </p>
<center style="margin:0;"> 

**CIM**, **EAU**, **FAC**, **REGIE**, et le petit dernier **CIM-FR**. 
<center/>
<p style="margin-top:0;">
**CIM** est le dernier module mit en production début 2025, de nombreuses evolutions logicielles ont été rescencés pour répondre
au besoin des adhérents. \
**CIM-FR** est un module un peu particulier, il ne correspond pas à un logiciel utilisé uniquement par la collectivité. C'est un module
visant à rendre accessible les informations d'un cimetière, *emplacement, défunts, date décès, concessionaire, ...*, à travers un 
portail web grand public.
</p>

Dû aux nombreuses évolutions à apporter, c'est sur ces 2 modules que j'ai eu l'occasion de travailler. En effet l'unité se concentre en priorité
sur le support (bugs et modifications) et la migration des nouvelles collectivités des 4 modules en production. \
Ceci explique en pourquoi un stagiaire est intéressant, il offre du temps *"bonus"* qui peut être alloué sur l'évolution des modules.

C'est dans ce contexte que j'ai rejoins AGEDI en tant que stagiaire pour un peu plus de 2 mois.

## 3. Présenter de manière concise et compréhensible par une personne non spécialiste du domaine la problématique de votre stage, et définir avec précision (environs 5 pages)

## 4. Votre méthodologie de travail pour la mise en œuvre de votre solution (environ 6 pages)

## 5. Expliquer les résultats obtenus et analyser leur cohérence (environ 2 pages)

## 6. Faire un bilan personnel du stage (1 page)

## 7. Présenter les obstacles, les points durs majeurs et les moyens mis en oeuvre pour les résoudre ; les compétences interpersonnelles acquises en entreprise

## 8. Conclure en répondant aux questions posées dans l'introduction, en résumant les principaux résultats à retenir et en indiquant les perspectives (2-3 paragraphes)


<style>
	body {
		margin-right: 50px;
		margin-left: 50px;
	}
	
	p, li {
		/* A noter que la taille de police ne sont pas équivalentes à celles sur un .docx.
		La taille de police à été testé pour être équivalente à une page docx en police 'Arial 12px' */
		font-size: 16px;
		line-height: 25px;
		text-align: justify;
	}
</style>







