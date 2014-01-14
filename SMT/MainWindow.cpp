#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	ui->paneBox->setCurrentIndex(0);
	ui->paneBox->setMinimumWidth(ui->projectTree->width()+6);

	currentProject = 0;
	displayOptionsDialog = 0;

	// Create GLPanel status bar and all labels
	glStatusBar = new QStatusBar();

	mouseXLabel = new QLabel("<b>X:</b> - ");
	mouseYLabel = new QLabel("<b>Y:</b> - ");

	numNodesLabel = new QLabel("<b>Nodes:</b> -       ");
	numElementsLabel = new QLabel("<b>Elements:</b> -       ");
	numTSLabel = new QLabel("<b>Timesteps:</b> -");

	// Stylize GL Panel status bar stuff
	glStatusBar->setSizeGripEnabled(false);
	glStatusBar->setStyleSheet("QStatusBar::item {border: 0px solid black };");

	// Add it all to the UI
	ui->GLPanelStatusLayout->insertWidget(0, glStatusBar);
	glStatusBar->insertPermanentWidget(0, mouseXLabel);
	glStatusBar->insertPermanentWidget(1, mouseYLabel);
	glStatusBar->insertWidget(0, numNodesLabel);
	glStatusBar->insertWidget(1, numElementsLabel);
	glStatusBar->insertWidget(2, numTSLabel);

	// Hide the progress bar and bottom section
	ui->progressBar->hide();


	// Connect the buttons
	connect(ui->openProjectButton, SIGNAL(clicked()), this, SLOT(openProject()));

}

MainWindow::~MainWindow()
{
	if (currentProject)
		delete currentProject;

	delete ui;

	CheckForMemoryLeaks();
}


void MainWindow::CheckForMemoryLeaks()
{
	if (GLShader::GetNumShaders() != 0)
	{
		DEBUG("MEMORY LEAK: " << GLShader::GetNumShaders() << " GLShader objects not deleted");
	}
	if (Layer::GetNumLayers() != 0)
	{
		DEBUG("MEMORY LEAK: " << Layer::GetNumLayers() << " Layer objects not deleted");
	}
	if (GLCamera::GetNumCameras() != 0)
	{
		DEBUG("MEMORY LEAK: " << GLCamera::GetNumCameras() << " GLCamera objects not deleted");
	}
}


void MainWindow::CreateProject(bool currentProjectFile)
{
	if (currentProject)
		delete currentProject;

	if (currentProjectFile)
		{
			currentProject = new Project(this);
		} else {
			QStringList selections;
			QFileDialog dialog(0, "Open an ADCIRC Subdomain Project", QDir::homePath());
			dialog.setModal(true);
			dialog.setNameFilter("ADCIRC Subdomain Projects (*.spf)");
			dialog.setFileMode(QFileDialog::ExistingFile);
			if (dialog.exec())
			{
				selections = dialog.selectedFiles();
				if (!selections.isEmpty())
				{
					currentProject = new Project(selections.first(), this);
				}
			} else {
				return;
			}
		}

		currentProject->SetOpenGLPanel(ui->GLPanel);
		currentProject->SetProgressBar(ui->progressBar);
		currentProject->SetProjectTree(ui->projectTree);

		/* Subdomain Creation */
		connect(ui->createSubdomainButton, SIGNAL(clicked()), currentProject, SLOT(CreateNewSubdomain()));

		/* Selection Tools */
		connect(ui->selectNodesCircle, SIGNAL(clicked()), currentProject, SLOT(SelectFullDomainCircleElements()));
		connect(ui->selectElementSingle, SIGNAL(clicked()), currentProject, SLOT(SelectFullDomainClickElements()));
		connect(ui->selectNodeSingle, SIGNAL(clicked()), currentProject, SLOT(SelectFullDomainPolygonElements()));
		connect(ui->selectNodesSquare, SIGNAL(clicked()), currentProject, SLOT(SelectFullDomainRectangleElements()));

		connect(ui->undoButton, SIGNAL(clicked()), currentProject, SLOT(Undo()));
		connect(ui->redoButton, SIGNAL(clicked()), currentProject, SLOT(Redo()));

		/* U/I Updates */
		connect(currentProject, SIGNAL(mouseX(float)), this, SLOT(showMouseX(float)));
		connect(currentProject, SIGNAL(mouseY(float)), this, SLOT(showMouseY(float)));
		connect(currentProject, SIGNAL(undoAvailable(bool)), ui->undoButton, SLOT(setEnabled(bool)));
		connect(currentProject, SIGNAL(redoAvailable(bool)), ui->redoButton, SLOT(setEnabled(bool)));
		connect(currentProject, SIGNAL(numElements(int)), this, SLOT(showNumElements(int)));
		connect(currentProject, SIGNAL(numNodes(int)), this, SLOT(showNumNodes(int)));
		connect(currentProject, SIGNAL(numElementsSelected(int)), this, SLOT(showNumSelectedElements(int)));
		connect(currentProject, SIGNAL(numNodesSelected(int)), this, SLOT(showNumSelectedNodes(int)));
		connect(currentProject, SIGNAL(maxSelectedZ(float)), this, SLOT(showMaxSelectedZ(float)));
		connect(currentProject, SIGNAL(minSelectedZ(float)), this, SLOT(showMinSelectedZ(float)));
		connect(currentProject, SIGNAL(showProjectView()), this, SLOT(showProjectExplorerPane()));

		/* Running ADCIRC */
		connect(ui->actionFull_Domain, SIGNAL(triggered()), currentProject, SLOT(RunFullDomain()));
		connect(currentProject, SIGNAL(subdomainCreated(QString)), this, SLOT(addSubdomainToList(QString)));

		/* Update list with already created subdomains */
		QStringList currSubs = currentProject->GetSubdomainNames();
		for (int i=0; i<currSubs.size(); ++i)
			addSubdomainToList(currSubs.at(i));

}


void MainWindow::showMaxSelectedZ(float newZ)
{
	ui->createMaxElevation->setText(QString::number(newZ, 'f', 4));
}


void MainWindow::showMinSelectedZ(float newZ)
{
	ui->createMinElevation->setText(QString::number(newZ, 'f', 4));
}


void MainWindow::showMouseX(float newX)
{
	if (mouseXLabel)
		mouseXLabel->setText(QString("<b>X:</b> ").append(QString::number(newX, 'f', 4)).append("   "));
}


void MainWindow::showMouseY(float newY)
{
	if (mouseYLabel)
		mouseYLabel->setText(QString("<b>Y:</b> ").append(QString::number(newY, 'f', 4)).append("   "));
}


void MainWindow::showNumNodes(int numNodes)
{
	if (numNodesLabel)
		numNodesLabel->setText(QString("<b>Nodes:</b> ").append(QString::number(numNodes)).append("   "));
}


void MainWindow::showNumElements(int numElements)
{
	if (numElementsLabel)
		numElementsLabel->setText(QString("<b>Elements:</b> ").append(QString::number(numElements)).append("   "));
}


void MainWindow::showNumTS(int numTS)
{
	if (numTSLabel)
		numTSLabel->setText(QString("<b>Timesteps:</b> ").append(QString::number(numTS)).append("   "));
}


void MainWindow::showNumSelectedNodes(int numNodes){
	ui->createNumNodesSelected->setText(QString::number(numNodes));
}


void MainWindow::showNumSelectedElements(int numElements)
{
	ui->createNumElementsSelected->setText(QString::number(numElements));
}


void MainWindow::showCircleStats(float x, float y, float rad)
{
	if (glStatusBar)
		glStatusBar->showMessage(QString("Center: ").append(QString::number(x, 'f', 5)).append(", ").append(QString::number(y, 'f', 5)).append("   Radius: ").append(QString::number(rad, 'f', 5)));
}


void MainWindow::openProject()
{
	CreateProject(false);
}


void MainWindow::addSubdomainToList(QString s)
{
	QListWidgetItem* newItem = new QListWidgetItem(s);
	newItem->setFlags(newItem->flags() | Qt::ItemIsUserCheckable);
	newItem->setCheckState(Qt::Unchecked);
	ui->subdomainsToRunList->addItem(newItem);
}


void MainWindow::showProjectExplorerPane()
{
	ui->paneBox->setCurrentIndex(0);
}


void MainWindow::showCreateSubdomainPane()
{
	ui->paneBox->setCurrentIndex(1);
}


void MainWindow::showEditSubdomainPane()
{
	ui->paneBox->setCurrentIndex(2);
}


void MainWindow::showAdcircPane()
{
	ui->paneBox->setCurrentIndex(3);
}


void MainWindow::showAnalyzeResultsPane()
{
	ui->paneBox->setCurrentIndex(4);
}
