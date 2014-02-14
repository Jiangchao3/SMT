#ifndef SUBDOMAIN_H
#define SUBDOMAIN_H

#include <iostream>
#include <set>

#include <QObject>
#include <QProgressBar>

#include "Project/Domains/Domain.h"

#include "Layers/SelectionLayers/SubDomainSelectionLayer.h"

#include "Project/Files/BNList14.h"
#include "Project/Files/Fort13.h"
#include "Project/Files/Fort14.h"
#include "Project/Files/Fort15.h"
#include "Project/Files/Fort22.h"
#include "Project/Files/Fort63.h"
#include "Project/Files/Fort64.h"
#include "Project/Files/Fort015.h"
#include "Project/Files/Fort020.h"
#include "Project/Files/Fort021.h"
#include "Project/Files/Fort022.h"
#include "Project/Files/Maxele63.h"
#include "Project/Files/Maxvel63.h"
#include "Project/Files/ProjectFile.h"
#include "Project/Files/Py140.h"
#include "Project/Files/Py141.h"

#include "OpenGL/GLCamera.h"

class SubDomain : public Domain
{
		Q_OBJECT
	public:
		SubDomain(QString domainName, ProjectFile *projectFile, QObject *parent=0);
		~SubDomain();

		virtual bool	IsFullDomain();

		Fort015*	GetFort015();
		Py140*		GetPy140();
		virtual QString	GetPath();
		QString		GetDomainName();
		Node*		GetSelectedNode();

		void	ResetNodalValues(unsigned int nodeNumber, Fort14 *fullDomainFort14);
		void	ResetAllNodalValues(Fort14 *fullDomainFort14);
		void	SaveAllChanges();
		void	SetNodalValues(unsigned int nodeNumber, QString x, QString y, QString z);

	private:

		SubDomainSelectionLayer*	selectionLayerSubdomain;

		BNList14*	bnList;
		QString		domainName;
		Fort13*		fort13;
		Fort15*		fort15;
		Fort22*		fort22;
		Fort63*		fort63;
		Fort64*		fort64;
		Fort015*	fort015;
		Fort020*	fort020;
		Fort021*	fort021;
		Fort022*	fort022;
		Maxele63*	maxele;
		Maxvel63*	maxvel;
		Py140*		py140;
		Py141*		py141;

		std::set<unsigned int>	editedNodes;

		void	CreateAllFiles();

	signals:

		void	editNode(unsigned int, QString, QString, QString);

};

#endif // SUBDOMAIN_H
