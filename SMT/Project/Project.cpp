#include "Project.h"

Project::Project(QObject *parent) :
	QObject(parent),
	displayOptions(0),
	editSubdomainList(0),
	fullDomain(0),
	fullDomainRunner(0),
	glPanel(0),
	progressBar(0),
	projectFile(0),
	projectInitialized(false),
	projectTree(0),
	subDomains(),
	visibleDomain(0)
{
	CreateProjectFile();
	if (IsInitialized())
		Initialize();
}


Project::Project(QString projectFile, QObject *parent) :
	QObject(parent),
	displayOptions(0),
	editSubdomainList(0),
	fullDomain(0),
	fullDomainRunner(0),
	glPanel(0),
	progressBar(0),
	projectFile(0),
	projectInitialized(false),
	projectTree(0),
	subDomains(),
	visibleDomain(0)
{
	OpenProjectFile(projectFile);
	if (IsInitialized())
		Initialize();
}


Project::~Project()
{
	if (glPanel)
		glPanel->SetActiveDomainNew(0);
	if (projectFile)
		delete projectFile;
	if (displayOptions)
		delete displayOptions;
	if (fullDomainRunner)
		delete fullDomainRunner;
}


void Project::DisplayDomain(int index)
{
	if (index == 0 && fullDomain)
	{
		SetVisibleDomain(fullDomain);
		if (projectTree)
		{
			projectTree->setCurrentItem(projectTree->findItems("Full Domain", Qt::MatchExactly | Qt::MatchRecursive).first());
		}
	}
	else if (index <= subDomains.size())
	{
		SubDomain *targetDomain = subDomains.at(index-1);
		if (targetDomain)
		{
			QString name = targetDomain->GetDomainName();
			SetVisibleDomain(subDomains.at(index-1));
			if (projectTree)
			{
				projectTree->setCurrentItem(projectTree->findItems(name, Qt::MatchExactly | Qt::MatchRecursive).first());
			}
		}
	}
}


bool Project::IsInitialized()
{
	return projectInitialized;
}


void Project::SetNodalValues(QString subdomainName, unsigned int nodeNumber, QString x, QString y, QString z)
{
	SubDomain *currentSubdomain;
	for (std::vector<SubDomain*>::iterator it = subDomains.begin();
	     it != subDomains.end();
	     ++it)
	{
		currentSubdomain = *it;
		if (currentSubdomain)
		{
			if (currentSubdomain->GetDomainName() == subdomainName)
			{
				currentSubdomain->SetNodalValues(nodeNumber, x, y, z);
				break;
			}
		}
	}
}


void Project::SetOpenGLPanel(OpenGLPanel *newPanel)
{
	if (newPanel)
	{
		glPanel = newPanel;
		connect(this, SIGNAL(subdomainCreated(QString)), glPanel, SLOT(addSubdomainToMenus(QString)));

		connect(glPanel, SIGNAL(openColorOptions()), this, SLOT(ShowDisplayOptionsDialog()));
		connect(glPanel, SIGNAL(matchColors(QAction*)), this, SLOT(MatchColors(QAction*)));
		connect(glPanel, SIGNAL(matchCamera(QAction*)), this, SLOT(MatchCamera(QAction*)));

		for (std::vector<SubDomain*>::iterator it = subDomains.begin(); it != subDomains.end(); ++it)
		{
			SubDomain* currSubdomain = *it;
			emit subdomainCreated(currSubdomain->GetDomainName());
		}
	}
}


void Project::SetProgressBar(QProgressBar *newBar)
{
	progressBar = newBar;

	if (fullDomain)
		fullDomain->SetProgressBar(newBar);

	SubDomain *currDomain = 0;
	for (std::vector<SubDomain*>::iterator it = subDomains.begin(); it != subDomains.end(); ++it)
	{
		currDomain = *it;
		if (currDomain)
			currDomain->SetProgressBar(newBar);
	}
}


void Project::SetProjectTree(QTreeWidget *newTree)
{
	if (newTree)
	{
		projectTree = newTree;
		PopulateProjectTree();
		connect(projectTree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
			this, SLOT(ProjectTreeItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
	}
}


void Project::SetEditSubdomainList(QListWidget *newList)
{
	if (newList)
	{
		editSubdomainList = newList;
		connect(editSubdomainList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
			this, SLOT(ProjectTreeItemChanged(QListWidgetItem*,QListWidgetItem*)));
	}
}


QStringList Project::GetSubdomainNames()
{
	QStringList list;
	for (std::vector<SubDomain*>::iterator currSub = subDomains.begin(); currSub != subDomains.end(); ++currSub)
		if (*currSub)
			list.append((*currSub)->GetDomainName());
	return list;
}


FullDomain* Project::BuildFullDomain()
{
	FullDomain *newFullDomain = new FullDomain(projectFile, this);

	connect(newFullDomain, SIGNAL(mouseX(float)), this, SIGNAL(mouseX(float)));
	connect(newFullDomain, SIGNAL(mouseY(float)), this, SIGNAL(mouseY(float)));
	connect(newFullDomain, SIGNAL(undoAvailable(bool)), this, SIGNAL(undoAvailable(bool)));
	connect(newFullDomain, SIGNAL(redoAvailable(bool)), this, SIGNAL(redoAvailable(bool)));
	connect(newFullDomain, SIGNAL(numElementsSelected(int)), this, SIGNAL(numElementsSelected(int)));
	connect(newFullDomain, SIGNAL(numNodesSelected(int)), this, SIGNAL(numNodesSelected(int)));
	connect(newFullDomain, SIGNAL(maxSelectedZ(float)), this, SIGNAL(maxSelectedZ(float)));
	connect(newFullDomain, SIGNAL(minSelectedZ(float)), this, SIGNAL(minSelectedZ(float)));

	return newFullDomain;
}


SubDomain* Project::BuildSubdomain(QString subdomainName)
{
	SubDomain *newSubdomain = new SubDomain(subdomainName, projectFile, this);

	connect(newSubdomain, SIGNAL(mouseX(float)), this, SIGNAL(mouseX(float)));
	connect(newSubdomain, SIGNAL(mouseY(float)), this, SIGNAL(mouseY(float)));
	connect(newSubdomain, SIGNAL(undoAvailable(bool)), this, SIGNAL(undoAvailable(bool)));
	connect(newSubdomain, SIGNAL(redoAvailable(bool)), this, SIGNAL(redoAvailable(bool)));
	connect(newSubdomain, SIGNAL(editNode(uint,QString,QString,QString)),
		this, SIGNAL(editNode(uint,QString,QString,QString)));

	if (progressBar)
		newSubdomain->SetProgressBar(progressBar);

	subDomains.push_back(newSubdomain);
	emit subdomainCreated(subdomainName);
	return newSubdomain;
}


void Project::CreateAllSubdomains()
{
	if (projectFile)
	{
		QStringList subdomainNames = projectFile->GetSubDomainNames();
		for (int i = 0; i<subdomainNames.size(); ++i)
		{
			BuildSubdomain(subdomainNames.at(i));
		}
	}
}


void Project::CreateProjectFile()
{
	if (!projectFile)
	{
		projectFile = new ProjectFile();
		CreateProjectDialog dialog;
		dialog.setModal(true);
		if (dialog.exec() && projectFile)
		{
			if (projectFile->CreateProjectFile(dialog.GetProjectDirectory(), dialog.GetProjectName()))
			{
				projectFile->SetFullDomainDirectory(dialog.GetProjectDirectory() + QDir::separator() + dialog.GetProjectName());

				QString fort10Loc = dialog.GetFort10Location();
				QString fort11Loc = dialog.GetFort11Location();
				QString fort13Loc = dialog.GetFort13Location();
				QString fort14Loc = dialog.GetFort14Location();
				QString fort15Loc = dialog.GetFort15Location();
				QString fort19Loc = dialog.GetFort19Location();
				QString fort20Loc = dialog.GetFort20Location();
				QString fort22Loc = dialog.GetFort22Location();
				QString fort23Loc = dialog.GetFort23Location();
				QString fort24Loc = dialog.GetFort24Location();
				if (!fort10Loc.isEmpty())
					projectFile->SetFullDomainFort10(fort10Loc, dialog.GetUseSymbolicLinkFort10());
				if (!fort11Loc.isEmpty())
					projectFile->SetFullDomainFort11(fort11Loc, dialog.GetUseSymbolicLinkFort11());
				if (!fort13Loc.isEmpty())
					projectFile->SetFullDomainFort13(fort13Loc, dialog.GetUseSymbolicLinkFort13());
				if (!fort14Loc.isEmpty())
					projectFile->SetFullDomainFort14(fort14Loc, dialog.GetUseSymbolicLinkFort14());
				if (!fort15Loc.isEmpty())
					projectFile->SetFullDomainFort15(fort15Loc, dialog.GetUseSymbolicLinkFort15());
				if (!fort19Loc.isEmpty())
					projectFile->SetFullDomainFort19(fort19Loc, dialog.GetUseSymbolicLinkFort19());
				if (!fort20Loc.isEmpty())
					projectFile->SetFullDomainFort20(fort20Loc, dialog.GetUseSymbolicLinkFort20());
				if (!fort22Loc.isEmpty())
					projectFile->SetFullDomainFort22(fort22Loc, dialog.GetUseSymbolicLinkFort22());
				if (!fort23Loc.isEmpty())
					projectFile->SetFullDomainFort23(fort23Loc, dialog.GetUseSymbolicLinkFort23());
				if (!fort24Loc.isEmpty())
					projectFile->SetFullDomainFort24(fort24Loc, dialog.GetUseSymbolicLinkFort24());
				projectFile->SaveProject();
			}

			fullDomain = BuildFullDomain();
			CreateAllSubdomains();

			projectInitialized = true;
		}
	}
}


Domain* Project::DetermineSelectedDomain(QTreeWidgetItem *item)
{
	if (item)
	{
		QString itemText = item->data(0, Qt::DisplayRole).toString();
		if (itemText == QString("Full Domain"))
		{
			return fullDomain;
		}
		QTreeWidgetItem *parentItem = item->parent();
		if (parentItem)
		{
			QString parentText = parentItem->data(0, Qt::DisplayRole).toString();
			if (parentText == QString("Full Domain"))
			{
				return fullDomain;
			}
			else if (parentText == QString("Sub Domains"))
			{
				for (std::vector<SubDomain*>::iterator currSub = subDomains.begin();
				     currSub != subDomains.end();
				     ++currSub)
				{
					if ((*currSub)->GetDomainName() == itemText)
						return *currSub;
				}
			}
			else
			{
					return DetermineSelectedDomain(parentItem);
			}
		}
	}

	return 0;
}


Domain* Project::DetermineSelectedDomain(QListWidgetItem *item)
{
	if (item)
	{
		QString itemText = item->text();
		return DetermineDomain(itemText);
	}

	return 0;
}


Domain* Project::DetermineDomain(QString domainName)
{
	if (domainName == QString("Full Domain"))
		return fullDomain;
	for (std::vector<SubDomain*>::iterator currSub = subDomains.begin();
	     currSub != subDomains.end();
	     ++currSub)
	{
		if ((*currSub)->GetDomainName() == domainName)
			return *currSub;
	}
	return 0;
}


SubDomain* Project::DetermineSubdomain(Domain *domain)
{
	for (std::vector<SubDomain*>::iterator currSub = subDomains.begin();
	     currSub != subDomains.end();
	     ++currSub)
	{
		if ((*currSub) == domain)
			return *currSub;
	}
	return 0;
}


void Project::Initialize()
{
	displayOptions = new DisplayOptionsDialog();
	fullDomainRunner = new FullDomainRunner();
}


void Project::OpenProjectFile(QString filePath)
{
	if (!projectFile)
	{
		projectFile = new ProjectFile();
		if (!projectFile->OpenProjectFile(filePath))
		{
			delete projectFile;
			projectFile = 0;
		} else {
			fullDomain = BuildFullDomain();
			CreateAllSubdomains();
			projectInitialized = true;
		}
	}
}


void Project::PopulateProjectTree()
{
	if (projectTree && projectFile)
	{
		projectTree->clear();
		projectTree->setColumnCount(2);
//		projectTree->setColumnWidth(0, projectTree->width()-20);
//		projectTree->setColumnWidth(1, 20);
		projectTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		projectTree->header()->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
		projectTree->header()->setStretchLastSection(false);
		projectTree->header()->resizeSection(0, projectTree->header()->width()-25);
		projectTree->header()->resizeSection(1, 25);

		QString projectName = projectFile->GetProjectName();
		QString projectDir = projectFile->GetProjectDirectory();
		QString fDir = projectFile->GetProjectDirectory();
		QString f14 = projectFile->GetFullDomainFort14();
		QString f15 = projectFile->GetFullDomainFort15();
		QString f63 = projectFile->GetFullDomainFort63();
		QString f64 = projectFile->GetFullDomainFort64();
		QString f015 = projectFile->GetFullDomainFort015();
		QString f065 = projectFile->GetFullDomainFort065();
		QString f066 = projectFile->GetFullDomainFort066();
		QString f067 = projectFile->GetFullDomainFort067();

		QFont boldFont;
		boldFont.setBold(true);

		QTreeWidgetItem *treeTop = new QTreeWidgetItem(projectTree);
		treeTop->setData(0, Qt::DisplayRole, projectName);
		treeTop->setData(0, Qt::FontRole, boldFont);
		treeTop->setData(0, Qt::StatusTipRole, projectName + " - " + projectDir);

		if (!fDir.isEmpty())
		{
			QTreeWidgetItem *fullTop = new QTreeWidgetItem(treeTop);
			fullTop->setData(0, Qt::DisplayRole, QString("Full Domain"));
			fullTop->setData(0, Qt::StatusTipRole, "Full Domain - " + projectDir);
//			fullTop->setIcon(1, QIcon(":/icons/images/icons/16_16/check.png"));

			if (!f14.isEmpty())
			{
				QTreeWidgetItem *item14 = new QTreeWidgetItem(fullTop);
				item14->setData(0, Qt::DisplayRole, QString("fort.14"));
				item14->setData(0, Qt::StatusTipRole, f14);
			}

			if (!f15.isEmpty())
			{
				QTreeWidgetItem *item15 = new QTreeWidgetItem(fullTop);
				item15->setData(0, Qt::DisplayRole, QString("fort.15"));
				item15->setData(0, Qt::StatusTipRole, f15);
			}

			if (!f63.isEmpty())
			{
				QTreeWidgetItem *item63 = new QTreeWidgetItem(fullTop);
				item63->setData(0, Qt::DisplayRole, QString("fort.63"));
				item63->setData(0, Qt::StatusTipRole, f63);
			}

			if (!f64.isEmpty())
			{
				QTreeWidgetItem *item64 = new QTreeWidgetItem(fullTop);
				item64->setData(0, Qt::DisplayRole, QString("fort.64"));
				item64->setData(0, Qt::StatusTipRole, f64);
			}

			if (!f015.isEmpty())
			{
				QTreeWidgetItem *item015 = new QTreeWidgetItem(fullTop);
				item015->setData(0, Qt::DisplayRole, QString("fort.015"));
				item015->setData(0, Qt::StatusTipRole, f015);
			}

			if (!f065.isEmpty())
			{
				QTreeWidgetItem *item065 = new QTreeWidgetItem(fullTop);
				item065->setData(0, Qt::DisplayRole, QString("fort.065"));
				item065->setData(0, Qt::StatusTipRole, f065);
			}

			if (!f066.isEmpty())
			{
				QTreeWidgetItem *item066 = new QTreeWidgetItem(fullTop);
				item066->setData(0, Qt::DisplayRole, QString("fort.066"));
				item066->setData(0, Qt::StatusTipRole, f066);
			}

			if (!f067.isEmpty())
			{
				QTreeWidgetItem *item067 = new QTreeWidgetItem(fullTop);
				item067->setData(0, Qt::DisplayRole, QString("fort.067"));
				item067->setData(0, Qt::StatusTipRole, f067);
			}
		}

		QTreeWidgetItem *subTop = new QTreeWidgetItem(treeTop);
		subTop->setData(0, Qt::DisplayRole, "Sub Domains");

		QStringList subDomainNames = projectFile->GetSubDomainNames();
		for (int i=0; i<subDomainNames.size(); ++i)
		{
			QString currName = subDomainNames.at(i);
			QString	currDir = projectFile->GetSubDomainDirectory(currName);
			QString sBN = projectFile->GetSubDomainBNList(currName);
			QString s14 = projectFile->GetSubDomainFort14(currName);
			QString s15 = projectFile->GetSubDomainFort15(currName);
			QString s63 = projectFile->GetSubDomainFort63(currName);
			QString s64 = projectFile->GetSubDomainFort64(currName);
			QString s015 = projectFile->GetSubDomainFort015(currName);
			QString s019 = projectFile->GetSubDomainFort019(currName);
			QString s020 = projectFile->GetSubDomainFort020(currName);
			QString s021 = projectFile->GetSubDomainFort021(currName);
			QString sME = projectFile->GetSubDomainMaxele(currName);
			QString sMV = projectFile->GetSubDomainMaxvel(currName);
			QString s140 = projectFile->GetSubDomainPy140(currName);
			QString s141 = projectFile->GetSubDomainPy141(currName);

			QTreeWidgetItem *currSub = new QTreeWidgetItem(subTop);
			currSub->setData(0, Qt::DisplayRole, currName);
			currSub->setData(0, Qt::StatusTipRole, currName + " - " + currDir);
//			currSub->setIcon(1, QIcon(":/icons/images/icons/16_16/refresh.png"));

			if (!sBN.isEmpty())
			{
				QTreeWidgetItem *itemBN = new QTreeWidgetItem(currSub);
				itemBN->setData(0, Qt::DisplayRole, QString("bnList.14"));
				itemBN->setData(0, Qt::StatusTipRole, sBN);
			}

			if (!s14.isEmpty())
			{
				QTreeWidgetItem *item14 = new QTreeWidgetItem(currSub);
				item14->setData(0, Qt::DisplayRole, QString("fort.14"));
				item14->setData(0, Qt::StatusTipRole, s14);
			}

			if (!s15.isEmpty())
			{
				QTreeWidgetItem *item15 = new QTreeWidgetItem(currSub);
				item15->setData(0, Qt::DisplayRole, QString("fort.15"));
				item15->setData(0, Qt::StatusTipRole, s15);
			}

			if (!s63.isEmpty())
			{
				QTreeWidgetItem *item63 = new QTreeWidgetItem(currSub);
				item63->setData(0, Qt::DisplayRole, QString("fort.63"));
				item63->setData(0, Qt::StatusTipRole, s63);
			}

			if (!s64.isEmpty())
			{
				QTreeWidgetItem *item64 = new QTreeWidgetItem(currSub);
				item64->setData(0, Qt::DisplayRole, QString("fort.64"));
				item64->setData(0, Qt::StatusTipRole, s64);
			}

			if (!s015.isEmpty())
			{
				QTreeWidgetItem *item015 = new QTreeWidgetItem(currSub);
				item015->setData(0, Qt::DisplayRole, QString("fort.015"));
				item015->setData(0, Qt::StatusTipRole, s015);
			}

			if (!s019.isEmpty())
			{
				QTreeWidgetItem *item019 = new QTreeWidgetItem(currSub);
				item019->setData(0, Qt::DisplayRole, QString("fort.019"));
				item019->setData(0, Qt::StatusTipRole, s019);
			}

			if (!s020.isEmpty())
			{
				QTreeWidgetItem *item020 = new QTreeWidgetItem(currSub);
				item020->setData(0, Qt::DisplayRole, QString("fort.020"));
				item020->setData(0, Qt::StatusTipRole, s020);
			}

			if (!s021.isEmpty())
			{
				QTreeWidgetItem *item021 = new QTreeWidgetItem(currSub);
				item021->setData(0, Qt::DisplayRole, QString("fort.021"));
				item021->setData(0, Qt::StatusTipRole, s021);
			}

			if (!sME.isEmpty())
			{
				QTreeWidgetItem *itemME = new QTreeWidgetItem(currSub);
				itemME->setData(0, Qt::DisplayRole, QString("maxele.63"));
				itemME->setData(0, Qt::StatusTipRole, sME);
			}

			if (!sMV.isEmpty())
			{
				QTreeWidgetItem *itemMV = new QTreeWidgetItem(currSub);
				itemMV->setData(0, Qt::DisplayRole, QString("maxvel.63"));
				itemMV->setData(0, Qt::StatusTipRole, sMV);
			}

			if (!s140.isEmpty())
			{
				QTreeWidgetItem *item140 = new QTreeWidgetItem(currSub);
				item140->setData(0, Qt::DisplayRole, QString("py.140"));
				item140->setData(0, Qt::StatusTipRole, s140);
			}

			if (!s141.isEmpty())
			{
				QTreeWidgetItem *item141 = new QTreeWidgetItem(currSub);
				item141->setData(0, Qt::DisplayRole, QString("py.141"));
				item141->setData(0, Qt::StatusTipRole, s141);
			}
		}
		projectTree->collapseAll();
		projectTree->expandToDepth(1);
	}
}


void Project::SetVisibleDomain(Domain *newDomain)
{
	if (newDomain)
	{
		visibleDomain = newDomain;
		if (glPanel)
			glPanel->SetActiveDomainNew(visibleDomain);

		Fort14 *currFort14 = visibleDomain->GetFort14();
		if (currFort14)
		{
			emit numElements(currFort14->GetNumElements());
			emit numNodes(currFort14->GetNumNodes());
		}

		emit undoAvailable(visibleDomain->UndoAvailable());
		emit redoAvailable(visibleDomain->RedoAvailable());

		if (displayOptions && displayOptions->isVisible())
		{
			displayOptions->SetActiveDomain(visibleDomain);
			displayOptions->update();
		}
	}
}


void Project::MatchFullCamera(SubDomain *targetSub)
{
	// Pick two nodes from the subdomain
	Node aSub, bSub, aFull, bFull;
	aSub.nodeNumber = 0;
	bSub.nodeNumber = 0;
	aFull.nodeNumber = 0;
	bFull.nodeNumber = 0;
	if (targetSub)
	{
		Fort14 *subFort = targetSub->GetFort14();
		if (subFort)
		{
			Node nodeA = subFort->GetNode(1);
			Node nodeB = subFort->GetNode(2);

			if (nodeA.nodeNumber)
				aSub = nodeA;
			if (nodeB.nodeNumber)
				bSub = nodeB;
		}
	}

	// Find their node numbers in the full domain
	if (aSub.nodeNumber && bSub.nodeNumber)
	{
		Fort14 *fullFort = fullDomain->GetFort14();
		Py140 *subPy = targetSub->GetPy140();
		if (fullFort && subPy)
		{
			Node nodeA = fullFort->GetNode(subPy->ConvertNewToOld(aSub.nodeNumber));
			Node nodeB = fullFort->GetNode(subPy->ConvertNewToOld(bSub.nodeNumber));

			if (nodeA.nodeNumber)
				aFull = nodeA;
			if (nodeB.nodeNumber)
				bFull = nodeB;
		}
	}

	// Create the scaling factor
	float subDist = aSub.normX - bSub.normX;
	float fullDist = aFull.normX - bFull.normX;
	float scaling = fullDist/subDist;

	// Create the offset factors
	float subMaxX = targetSub->GetFort14()->GetMaxX();
	float subMinX = targetSub->GetFort14()->GetMinX();
	float subMidX = subMinX + (subMaxX - subMinX) / 2.0;
	float subMaxY = targetSub->GetFort14()->GetMaxY();
	float subMinY = targetSub->GetFort14()->GetMinY();
	float subMidY = subMinY + (subMaxY - subMinY) / 2.0;
	float subMax = fmax(subMaxX-subMinX, subMaxY-subMinY);

	float fullMaxX = fullDomain->GetFort14()->GetMaxX();
	float fullMinX = fullDomain->GetFort14()->GetMinX();
	float fullMidX = fullMinX + (fullMaxX - fullMinX) / 2.0;
	float fullMaxY = fullDomain->GetFort14()->GetMaxY();
	float fullMinY = fullDomain->GetFort14()->GetMinY();
	float fullMidY = fullMinY + (fullMaxY - fullMinY) / 2.0;

	float xOffset = (subMidX - fullMidX) / subMax;
	float yOffset = (subMidY - fullMidY) / subMax;

	// Apply the new settings
	CameraSettings selectedSettings = fullDomain->GetCameraSettings();
	selectedSettings.zoomLevel *= scaling;
	selectedSettings.panX = selectedSettings.panX/scaling + xOffset;
	selectedSettings.panY = selectedSettings.panY/scaling + yOffset;
	visibleDomain->SetCameraSettings(selectedSettings);
}


void Project::MatchSubCamera(SubDomain *targetSub)
{
	// Pick two nodes from the subdomain
	Node aSub, bSub, aFull, bFull;
	aSub.nodeNumber = 0;
	bSub.nodeNumber = 0;
	aFull.nodeNumber = 0;
	bFull.nodeNumber = 0;
	if (targetSub)
	{
		Fort14 *subFort = targetSub->GetFort14();
		if (subFort)
		{
			Node nodeA = subFort->GetNode(1);
			Node nodeB = subFort->GetNode(2);

			if (nodeA.nodeNumber)
				aSub = nodeA;
			if (nodeB.nodeNumber)
				bSub = nodeB;
		}
	}

	// Find their node numbers in the full domain
	if (aSub.nodeNumber && bSub.nodeNumber)
	{
		Fort14 *fullFort = fullDomain->GetFort14();
		Py140 *subPy = targetSub->GetPy140();
		if (fullFort && subPy)
		{
			Node nodeA = fullFort->GetNode(subPy->ConvertNewToOld(aSub.nodeNumber));
			Node nodeB = fullFort->GetNode(subPy->ConvertNewToOld(bSub.nodeNumber));

			if (nodeA.nodeNumber)
				aFull = nodeA;
			if (nodeB.nodeNumber)
				bFull = nodeB;
		}
	}

	// Create the scaling factor
	float subDist = aSub.normX - bSub.normX;
	float fullDist = aFull.normX - bFull.normX;
	float scaling = fullDist/subDist;

	// Create the offset factors
	float subMaxX = targetSub->GetFort14()->GetMaxX();
	float subMinX = targetSub->GetFort14()->GetMinX();
	float subMidX = subMinX + (subMaxX - subMinX) / 2.0;
	float subMaxY = targetSub->GetFort14()->GetMaxY();
	float subMinY = targetSub->GetFort14()->GetMinY();
	float subMidY = subMinY + (subMaxY - subMinY) / 2.0;

	float fullMaxX = fullDomain->GetFort14()->GetMaxX();
	float fullMinX = fullDomain->GetFort14()->GetMinX();
	float fullMidX = fullMinX + (fullMaxX - fullMinX) / 2.0;
	float fullMaxY = fullDomain->GetFort14()->GetMaxY();
	float fullMinY = fullDomain->GetFort14()->GetMinY();
	float fullMidY = fullMinY + (fullMaxY - fullMinY) / 2.0;
	float fullMax = fmax(fullMaxX-fullMinX, fullMaxY-fullMinY);

	float xOffset = (subMidX - fullMidX) / fullMax;
	float yOffset = (subMidY - fullMidY) / fullMax;

	// Apply the new settings
	CameraSettings selectedSettings = targetSub->GetCameraSettings();
	selectedSettings.zoomLevel /= scaling;
	selectedSettings.panX = selectedSettings.panX*scaling - xOffset;
	selectedSettings.panY = selectedSettings.panY*scaling - yOffset;
	fullDomain->SetCameraSettings(selectedSettings);
}


void Project::MatchSubCamera(SubDomain *originSub, SubDomain *targetSub)
{
	MatchSubCamera(targetSub);
	MatchFullCamera(originSub);
}


void Project::CreateNewSubdomain()
{
	if (fullDomain && projectFile)
	{
		CreateSubdomainDialog dlg;
		dlg.SetDefaultDirectory(projectFile->GetFullDomainDirectory());
		if (dlg.exec())
		{
			QString name = dlg.GetSubdomainName();
			QString targetDir = dlg.GetTargetDirectory();
			int version = dlg.GetSubdomainVersion();
			int recordFrequency = dlg.GetRecordFrequency();

			if (!name.isEmpty() && !targetDir.isEmpty() && (version == 1 || version == 2) && recordFrequency > 0)
			{
				SubdomainCreator creator;
				bool newSubdomain = creator.CreateSubdomain(name, projectFile, targetDir, fullDomain, version, recordFrequency);
				if (newSubdomain)
				{
					BuildSubdomain(name);
					PopulateProjectTree();
					emit showProjectView();
					projectTree->setCurrentItem(projectTree->findItems(name, Qt::MatchExactly | Qt::MatchRecursive).first());
				}
			}
		}
	}
}


void Project::EditProjectSettings()
{

}


void Project::Redo()
{
	if (visibleDomain)
		visibleDomain->Redo();
}


void Project::RunFullDomain()
{
	if (!fullDomainRunner)
	{
		fullDomainRunner = new FullDomainRunner();
	}
	if (fullDomainRunner && fullDomain)
	{
		fullDomainRunner->SetFullDomain(fullDomain);
		if (fullDomainRunner->PrepareForFullDomainRun())
			fullDomainRunner->PerformFullDomainRun();
	}
}


void Project::RunSubdomain(QString subdomain)
{

}


void Project::SaveProject()
{

}


void Project::SelectFullDomainCircleElements()
{
	if (fullDomain)
		fullDomain->UseTool(CircleToolType, ElementSelection, Select);
}


void Project::SelectFullDomainClickElements()
{
	if (fullDomain)
		fullDomain->UseTool(ClickToolType, ElementSelection, Select);
}


void Project::SelectFullDomainPolygonElements()
{
	if (fullDomain)
		fullDomain->UseTool(PolygonToolType, ElementSelection, Select);
}


void Project::SelectFullDomainRectangleElements()
{
	if (fullDomain)
		fullDomain->UseTool(RectangleToolType, ElementSelection, Select);
}


void Project::DeselectFullDomainCircleElements()
{
	if (fullDomain)
		fullDomain->UseTool(CircleToolType, ElementSelection, Deselect);
}


void Project::DeselectFullDomainClickElements()
{
	if (fullDomain)
		fullDomain->UseTool(ClickToolType, ElementSelection, Deselect);
}


void Project::DeselectFullDomainPolygonElements()
{
	if (fullDomain)
		fullDomain->UseTool(PolygonToolType, ElementSelection, Deselect);
}


void Project::DeselectFullDomainRectangleElements()
{
	if (fullDomain)
		fullDomain->UseTool(RectangleToolType, ElementSelection, Deselect);
}


void Project::SelectSingleSubdomainNode()
{
	SubDomain *currentSub = 0;
	for (std::vector<SubDomain*>::iterator it = subDomains.begin(); it != subDomains.end(); ++it)
	{
		currentSub = *it;
		if (currentSub == visibleDomain)
		{
			currentSub->UseTool(ClickToolType, NodeSelection, Select);
			break;
		}
	}
}


void Project::ShowDisplayOptionsDialog()
{
	if (visibleDomain && displayOptions)
	{
		displayOptions->SetActiveDomain(visibleDomain);
		displayOptions->show();
		displayOptions->update();
	}
}


void Project::ToggleQuadtreeVisible()
{
	if (visibleDomain)
	{
		Fort14 *currFort14 = visibleDomain->GetFort14();
		if (currFort14)
			currFort14->ToggleQuadtreeVisible();
	}
}


void Project::Undo()
{
	if (visibleDomain)
		visibleDomain->Undo();
}


void Project::ProjectTreeItemChanged(QTreeWidgetItem *item, QTreeWidgetItem *)
{
	Domain *selectedDomain = DetermineSelectedDomain(item);
	if (selectedDomain && selectedDomain != visibleDomain)
	{
		SetVisibleDomain(selectedDomain);
	}
}


void Project::ProjectTreeItemChanged(QListWidgetItem *item, QListWidgetItem *)
{
	Domain *selectedDomain = DetermineSelectedDomain(item);
	if (selectedDomain && selectedDomain != visibleDomain)
	{
		SetVisibleDomain(selectedDomain);
	}
}


void Project::MatchColors(QAction *action)
{
	if (action)
	{
		QString selectedDomainName = action->text();
		Domain* selectedDomain = DetermineDomain(selectedDomainName);
		if (selectedDomain && visibleDomain)
		{
			// Get the active shader types
			ShaderType outlineType = selectedDomain->GetTerrainOutlineType();
			ShaderType fillType = selectedDomain->GetTerrainFillType();

			if (outlineType == SolidShaderType)
			{
				// Copy the solid outline color from the selected domain
				QColor solidOutline = selectedDomain->GetTerrainSolidOutline();
				visibleDomain->SetTerrainSolidOutline(solidOutline);
			}
			else if (outlineType == GradientShaderType)
			{
				// Copy the outline gradient from the selected domain
				// Convert percentages in gradient stops to elevation values
				// Convert these elevation values back into percentages in the range of the selected domain
				float selectedRange[2] = {selectedDomain->GetTerrainMinZ(), selectedDomain->GetTerrainMaxZ()};
				float currentRange[2] = {visibleDomain->GetTerrainMinZ(), visibleDomain->GetTerrainMaxZ()};
				QGradientStops selectedGradientOutline = selectedDomain->GetTerrainGradientOutline();
				QGradientStops newStopsOutline;
				for (int i=0; i<selectedGradientOutline.count(); ++i)
				{
					QGradientStop currentStop = selectedGradientOutline[i];
					float percentage = currentStop.first;
					QColor color = currentStop.second;

					float selectedValue = selectedRange[1] - percentage*(selectedRange[1] - selectedRange[0]);
					if (selectedValue >= currentRange[0] && selectedValue <= currentRange[1])
					{
						QGradientStop newStop;
						newStop.first = (currentRange[1] - selectedValue) / (currentRange[1] - currentRange[0]);
						newStop.second = color;
						newStopsOutline.append(newStop);
					}
				}
				visibleDomain->SetTerrainGradientOutline(newStopsOutline);
			}

			if (fillType == SolidShaderType)
			{
				// Copy the solid fill color from the selected domain
				QColor solidFill = selectedDomain->GetTerrainSolidFill();
				visibleDomain->SetTerrainSolidFill(solidFill);
			}
			else if (fillType == GradientShaderType)
			{
				// Copy the fill gradient from the selected domain
				// Convert percentages in gradient stops to elevation values
				// Convert these elevation values back into percentages in the range of the selected domain
				float selectedRange[2] = {selectedDomain->GetTerrainMinZ(), selectedDomain->GetTerrainMaxZ()};
				float currentRange[2] = {visibleDomain->GetTerrainMinZ(), visibleDomain->GetTerrainMaxZ()};
				QGradientStops selectedGradientFill = selectedDomain->GetTerrainGradientFill();
				QGradientStops newStopsFill;
				for (int i=0; i<selectedGradientFill.count(); ++i)
				{
					QGradientStop currentStop = selectedGradientFill[i];
					float percentage = currentStop.first;
					QColor color = currentStop.second;

					float selectedValue = selectedRange[1] - percentage*(selectedRange[1] - selectedRange[0]);
					if (selectedValue >= currentRange[0] && selectedValue <= currentRange[1])
					{
						QGradientStop newStop;
						newStop.first = (currentRange[1] - selectedValue) / (currentRange[1] - currentRange[0]);
						newStop.second = color;
						newStopsFill.append(newStop);
					}
				}
				visibleDomain->SetTerrainGradientFill(newStopsFill);
			}
		}
	}
}


void Project::MatchCamera(QAction *action)
{
	if (action)
	{
		QString selectedDomainName = action->text();
		Domain* selectedDomain = DetermineDomain(selectedDomainName);
		if (selectedDomain && visibleDomain)
		{
			if (selectedDomain == fullDomain && visibleDomain != fullDomain)
			{
				SubDomain *targetSub = DetermineSubdomain(visibleDomain);
				if (targetSub)
					MatchFullCamera(targetSub);
			}

			else if (selectedDomain != fullDomain && visibleDomain == fullDomain)
			{
				SubDomain *targetSub = DetermineSubdomain(selectedDomain);
				if (targetSub)
					MatchSubCamera(targetSub);
			}

			else if (selectedDomain != fullDomain && visibleDomain != fullDomain)
			{
				SubDomain *targetSub = DetermineSubdomain(selectedDomain);
				SubDomain *originSub = DetermineSubdomain(visibleDomain);
				if (targetSub && originSub)
					MatchSubCamera(originSub, targetSub);
			}
		}
	}
}
