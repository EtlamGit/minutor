/*
   Copyright (c) 2013, Sean Kasun
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QAction>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTreeWidget>
#include <QProgressDialog>
#include <QDir>
#include <QRegExp>
#include "minutor.h"
#include "mapview.h"
#include "labelledslider.h"
#include "nbt.h"
#include "json.h"
#include "definitionmanager.h"
#include "settings.h"
#include "dimensions.h"
#include "worldsave.h"
#include "properties.h"

Minutor::Minutor():
	maxentitydistance(0)
{
	mapview = new MapView;
	connect(mapview,     SIGNAL(hoverTextChanged(QString)),
			statusBar(), SLOT(showMessage(QString)));
	connect(mapview,     SIGNAL(showProperties(int,int,int)),
			this,        SLOT(showProperties(int,int,int)));
	dm=new DefinitionManager(this);
	mapview->attach(dm);
	connect(dm,   SIGNAL(packsChanged()),
			this, SLOT(updateDimensions()));
	dimensions=dm->dimensions();
	connect(dimensions, SIGNAL(dimensionChanged(Dimension &)),
			this,       SLOT(viewDimension(Dimension &)));
	settings = new Settings(this);
	connect(settings, SIGNAL(settingsUpdated()),
			this,     SLOT(rescanWorlds()));


	if (settings->autoUpdate)
		dm->autoUpdate();

	createActions();
	createMenus();
	createStatusBar();

	QBoxLayout *mainLayout;
	if (settings->verticalDepth)
	{
		mainLayout = new QHBoxLayout;
		depth = new LabelledSlider(Qt::Vertical);
		mainLayout->addWidget(mapview,1);
		mainLayout->addWidget(depth);
	} else {
		mainLayout = new QVBoxLayout;
		depth = new LabelledSlider(Qt::Horizontal);
		mainLayout->addWidget(depth);
		mainLayout->addWidget(mapview,1);
	}
	depth->setValue(255);
	mainLayout->setSpacing(0);
	mainLayout->setContentsMargins(0,0,0,0);

	connect(depth,   SIGNAL(valueChanged(int)),
	        mapview, SLOT(setDepth(int)));
	connect(mapview, SIGNAL(demandDepthChange(int)),
	        depth,   SLOT(changeValue(int)));
	connect(this,    SIGNAL(worldLoaded(bool)),
	        mapview, SLOT(setEnabled(bool)));
	connect(this,    SIGNAL(worldLoaded(bool)),
			depth,   SLOT(setEnabled(bool)));

	QWidget *central=new QWidget;
	central->setLayout(mainLayout);

	setCentralWidget(central);
	layout()->setContentsMargins(0,0,0,0);

	setWindowTitle(qApp->applicationName());

	propView = new Properties(this);

	emit worldLoaded(false);
}

void Minutor::openWorld()
{
	QAction *action=qobject_cast<QAction*>(sender());
	if (action)
		loadWorld(action->data().toString());
}

void Minutor::open()
{
	QString dirName = QFileDialog::getExistingDirectory(this,tr("Open World"));
	if (!dirName.isEmpty())
	{
		QDir path(dirName);
		if (!path.exists("level.dat"))
		{
			QMessageBox::warning(this,
								 tr("Couldn't open world"),
								 tr("%1 is not a valid Minecraft world").arg(dirName),
								 QMessageBox::Cancel);
			return;
		}
		loadWorld(dirName);
	}
}

void Minutor::reload()
{
	loadWorld(currentWorld);
}

void Minutor::save()
{
	QFileDialog fileDialog(this);
	fileDialog.setDefaultSuffix("png");
	QString filename = fileDialog.getSaveFileName(this,tr("Save world as PNG"),QString(),"*.png");
	if (!filename.isEmpty())
	{
		WorldSave *ws=new WorldSave(filename,mapview);
		progress=new QProgressDialog();
		progress->setCancelButton(NULL);
		progress->setMaximum(100);
		progress->show();
		connect(ws,   SIGNAL(progress(QString,double)),
				this, SLOT(saveProgress(QString,double)));
		connect(ws,   SIGNAL(finished()),
				this, SLOT(saveFinished()));
		QThreadPool::globalInstance()->start(ws);
	}
}

void Minutor::saveProgress(QString status, double value)
{
	progress->setValue(value*100);
	progress->setLabelText(status);
}
void Minutor::saveFinished()
{
	progress->hide();
	delete progress;
}

void Minutor::closeWorld()
{
	locations.clear();
	for (int i=0;i<players.size();i++)
	{
		jumpMenu->removeAction(players[i]);
		delete players[i];
	}
	players.clear();
	jumpMenu->setEnabled(false);
	dimensions->removeDimensions(dimMenu);
	currentWorld=QString();
	emit worldLoaded(false);
}

void Minutor::jumpToLocation()
{
	QAction *action=qobject_cast<QAction*>(sender());
	if (action)
	{
		Location loc=locations[action->data().toInt()];
		mapview->setLocation(loc.x,loc.z);
	}
}

void Minutor::jumpToXZ(int blockX, int blockZ)
{
	mapview->setLocation(blockX, blockZ);
}

void Minutor::toggleFlags()
{
	int flags = 0;

	if (lightingAct->isChecked())     flags |= MapView::flgLighting;
	if (mobSpawnAct->isChecked())     flags |= MapView::flgMobSpawn;
	if (caveModeAct->isChecked())     flags |= MapView::flgCaveMode;
	if (depthShadingAct->isChecked()) flags |= MapView::flgDepthShading;
	mapview->setFlags(flags);

	mapview->clearStructureFilter();
	QList<QAction*>::iterator it, itEnd = structureActions.end();
	for (it = structureActions.begin(); it != itEnd; ++it)
	{
		if ((*it)->isChecked())
		{
			mapview->addStructureFilter((*it)->data().toString());
		}
	}

	mapview->clearEntityFilter();
	if (entityHostileAct->isChecked()) mapview->addEntityFilter(Entity::ECAT::ET_HOSTILE);
	if (entityPassiveAct->isChecked()) mapview->addEntityFilter(Entity::ECAT::ET_PASSIVE);
	if (entityItemAct->isChecked())    mapview->addEntityFilter(Entity::ECAT::ET_ITEM);
	if (entityOtherAct->isChecked())   mapview->addEntityFilter(Entity::ECAT::ET_OTHER);

	mapview->redraw();
}

void Minutor::viewDimension(Dimension &dim)
{
	mapview->setDimension(dim.path,dim.scale);
}

void Minutor::about()
{
	QMessageBox::about(this,tr("About %1").arg(qApp->applicationName()),
					   tr("<b>%1</b> v%2<br/>\n"
						  "&copy; Copyright %3, %4").arg(qApp->applicationName())
					   .arg(qApp->applicationVersion())
					   .arg(2013)
					   .arg(qApp->organizationName()));
}

void Minutor::updateDimensions()
{
	dimensions->getDimensions(currentWorld,dimMenu,this);
}

void Minutor::createActions()
{
	getWorldList();

	// [File]
	openAct = new QAction(tr("&Open..."),this);
	openAct->setShortcut(tr("Ctrl+O"));
	openAct->setStatusTip(tr("Open a world"));
	connect(openAct, SIGNAL(triggered()),
			this,    SLOT(open()));

	reloadAct = new QAction(tr("&Reload"),this);
	reloadAct->setShortcut(tr("F5"));
	reloadAct->setStatusTip(tr("Reload current world"));
	connect(reloadAct, SIGNAL(triggered()),
			this,      SLOT(reload()));
	connect(this,      SIGNAL(worldLoaded(bool)),
			reloadAct, SLOT(setEnabled(bool)));

	saveAct = new QAction(tr("&Save PNG..."),this);
	saveAct->setShortcut(tr("Ctrl+S"));
	saveAct->setStatusTip(tr("Save as PNG"));
	connect(saveAct, SIGNAL(triggered()),
			this,    SLOT(save()));
	connect(this,    SIGNAL(worldLoaded(bool)),
			saveAct, SLOT(setEnabled(bool)));

	exitAct = new QAction(tr("E&xit"),this);
	exitAct->setShortcut(tr("Ctrl+Q"));
	exitAct->setStatusTip(tr("Exit %1").arg(qApp->applicationName()));
	connect(exitAct, SIGNAL(triggered()),
			this,    SLOT(close()));

	// [View]
	jumpSpawnAct = new QAction(tr("Jump to &Spawn"),this);
	jumpSpawnAct->setShortcut(tr("F1"));
	jumpSpawnAct->setStatusTip(tr("Jump to world spawn"));
	connect(jumpSpawnAct, SIGNAL(triggered()),
			this,         SLOT(jumpToLocation()));
	connect(this,         SIGNAL(worldLoaded(bool)),
			jumpSpawnAct, SLOT(setEnabled(bool)));


	lightingAct = new QAction(tr("&Lighting"),this);
	lightingAct->setCheckable(true);
	lightingAct->setShortcut(tr("Ctrl+L"));
	lightingAct->setStatusTip(tr("Toggle lighting on/off"));
	connect(lightingAct, SIGNAL(triggered()),
			this,        SLOT(toggleFlags()));

	depthShadingAct = new QAction(tr("&Depth shading"), this);
	depthShadingAct->setCheckable(true);
	depthShadingAct->setShortcut(tr("Ctrl+D"));
	depthShadingAct->setStatusTip(tr("Toggle shading based on relative depth"));
	connect(depthShadingAct, SIGNAL(triggered()),
			this,            SLOT(toggleFlags()));

	mobSpawnAct = new QAction(tr("&Mob spawning"),this);
	mobSpawnAct->setCheckable(true);
	mobSpawnAct->setShortcut(tr("Ctrl+M"));
	mobSpawnAct->setStatusTip(tr("Toggle show mob spawning on/off"));
	connect(mobSpawnAct, SIGNAL(triggered()),
			this,        SLOT(toggleFlags()));

	caveModeAct = new QAction(tr("&Cave Mode"),this);
	caveModeAct->setCheckable(true);
	caveModeAct->setShortcut(tr("Ctrl+C"));
	caveModeAct->setStatusTip(tr("Toggle cave mode on/off"));
	connect(caveModeAct, SIGNAL(triggered()),
			this,        SLOT(toggleFlags()));
	caveModeAct->setEnabled(false);

	
	entityHostileAct = new QAction(tr("&Hostile"),this);
	entityHostileAct->setCheckable(true);
	entityHostileAct->setStatusTip(tr("Toggle hostile Mob overlay on/off"));
	connect(entityHostileAct, SIGNAL(triggered()),
			this,             SLOT(toggleFlags()));

	entityPassiveAct = new QAction(tr("&Passive"),this);
	entityPassiveAct->setCheckable(true);
	entityPassiveAct->setStatusTip(tr("Toggle passive Mob overlay on/off"));
	connect(entityPassiveAct, SIGNAL(triggered()),
			this,             SLOT(toggleFlags()));

	entityItemAct = new QAction(tr("&Items"),this);
	entityItemAct->setCheckable(true);
	entityItemAct->setStatusTip(tr("Toggle Item overlay on/off"));
	connect(entityItemAct, SIGNAL(triggered()),
			this,          SLOT(toggleFlags()));

	entityOtherAct = new QAction(tr("&Others"),this);
	entityOtherAct->setCheckable(true);
	entityOtherAct->setStatusTip(tr("Toggle other entities overlay on/off"));
	connect(entityOtherAct, SIGNAL(triggered()),
			this,           SLOT(toggleFlags()));
	
	
	manageDefsAct = new QAction(tr("Manage &Definitions..."),this);
	manageDefsAct->setStatusTip(tr("Manage block and biome definitions"));
	connect(manageDefsAct, SIGNAL(triggered()),
			dm,            SLOT(show()));

	refreshAct = new QAction(tr("Refresh"), this);
	refreshAct->setShortcut(tr("F2"));
	refreshAct->setStatusTip(tr("Reloads all chunks, but keeps the same position / dimension"));
	connect(refreshAct, SIGNAL(triggered()),
			mapview,    SLOT(clearCache()));

	// [Help]
	aboutAct = new QAction(tr("&About"),this);
	aboutAct->setStatusTip(tr("About %1").arg(qApp->applicationName()));
	connect(aboutAct, SIGNAL(triggered()),
			this,     SLOT(about()));

	settingsAct = new QAction(tr("Settings..."),this);
	settingsAct->setStatusTip(tr("Change %1 Settings").arg(qApp->applicationName()));
	connect(settingsAct, SIGNAL(triggered()),
			settings,    SLOT(show()));

	updatesAct = new QAction(tr("Check for updates..."),this);
	updatesAct->setStatusTip(tr("Check for updated packs"));
	connect(updatesAct, SIGNAL(triggered()),
			dm,         SLOT(checkForUpdates()));
}

void Minutor::createMenus()
{
	// [File]
	fileMenu=menuBar()->addMenu(tr("&File"));
	worldMenu=fileMenu->addMenu(tr("&Open World"));

	worldMenu->addActions(worlds);
	if (worlds.size()==0) //no worlds found
		worldMenu->setEnabled(false);

	fileMenu->addAction(openAct);
	fileMenu->addAction(reloadAct);
	fileMenu->addSeparator();
	fileMenu->addAction(saveAct);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAct);

	// [View]
	viewMenu=menuBar()->addMenu(tr("&View"));
	viewMenu->addAction(jumpSpawnAct);
	jumpMenu=viewMenu->addMenu(tr("&Jump to Player"));
	jumpMenu->setEnabled(false);
	dimMenu=viewMenu->addMenu(tr("&Dimension"));
	dimMenu->setEnabled(false);
	viewMenu->addSeparator();
	viewMenu->addAction(lightingAct);
	viewMenu->addAction(mobSpawnAct);
	viewMenu->addAction(caveModeAct);
	viewMenu->addAction(depthShadingAct);
	// [View->Structures]
	structuresMenu = viewMenu->addMenu(tr("S&tructures"));
	structuresMenu->setEnabled(false);
	// [View->Entities]
	entitiesMenu = viewMenu->addMenu(tr("&Entities"));
	entitiesMenu->addAction(entityHostileAct);
	entitiesMenu->addAction(entityPassiveAct);
	entitiesMenu->addAction(entityItemAct);
	entitiesMenu->addAction(entityOtherAct);

	viewMenu->addSeparator();
	viewMenu->addAction(refreshAct);
	viewMenu->addSeparator();
	viewMenu->addAction(manageDefsAct);

	menuBar()->addSeparator();

	// [Help]
	helpMenu=menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(aboutAct);
	helpMenu->addSeparator();
	helpMenu->addAction(settingsAct);
	helpMenu->addAction(updatesAct);
}

void Minutor::createStatusBar()
{
	statusBar()->showMessage(tr("Ready"));
}

QString Minutor::getWorldName(QDir path)
{
	if (!path.exists("level.dat")) //no level.dat?  no world
		return QString();

	NBT level(path.filePath("level.dat"));
	return level.at("Data")->at("LevelName")->toString();
}


void Minutor::getWorldList()
{
	QDir mc(settings->mcpath);
	if (!mc.cd("saves"))
		return;

	QDirIterator it(mc);
	int key=1;
	while (it.hasNext())
	{
		it.next();
		if (it.fileInfo().isDir())
		{
			QString name=getWorldName(it.filePath());
			if (!name.isNull())
			{
				QAction *w=new QAction(this);
				w->setText(name);
				w->setData(it.filePath());
				if (key<10)
				{
					w->setShortcut("Ctrl+"+QString::number(key));
					key++;
				}
				connect(w, SIGNAL(triggered()),
						this, SLOT(openWorld()));
				worlds.append(w);
			}
		}
	}
}

void Minutor::loadWorld(QDir path)
{
	closeWorld(); //just in case
	currentWorld=path;

	NBT level(path.filePath("level.dat"));

	Tag *data=level.at("Data");
	//add level name to window title
	setWindowTitle(qApp->applicationName()+" - "+data->at("LevelName")->toString());
	//save world spawn
	jumpSpawnAct->setData(locations.count());
	locations.append(Location(data->at("SpawnX")->toDouble(),
							  data->at("SpawnZ")->toDouble()));
	//show saved players
	if (path.cd("playerdata") || path.cd("players"))
	{
		QDirIterator it(path);
		bool hasPlayers=false;
		while (it.hasNext())
		{
			it.next();
			if (it.fileInfo().isFile())
			{
				hasPlayers=true;
				NBT player(it.filePath());
				Tag *pos=player.at("Pos");
				double posX=pos->at(0)->toDouble();
				double posZ=pos->at(2)->toDouble();
				Tag *dim=player.at("Dimension");
				if (dim && (dim->toInt() == -1))
				{
					posX *= 8;
					posZ *= 8;
				}
				QString playerName = it.fileInfo().completeBaseName();
				QRegExp id("[0-9a-z]{8,8}\\-[0-9a-z]{4,4}\\-[0-9a-z]{4,4}\\-[0-9a-z]{4,4}\\-[0-9a-z]{12,12}");
				if (id.exactMatch(playerName))
				{
					playerName = QString("Player %1").arg(players.length());
				}

				QAction *p=new QAction(this);
				p->setText(playerName);
				p->setData(locations.count());
				locations.append(Location(posX, posZ));
				connect(p, SIGNAL(triggered()),
						this, SLOT(jumpToLocation()));
				players.append(p);
				if (player.has("SpawnX")) //player has a bed
				{
					p=new QAction(this);


					p->setText(playerName+"'s Bed");
					p->setData(locations.count());
					locations.append(Location(player.at("SpawnX")->toDouble(),
											  player.at("SpawnZ")->toDouble()));
					connect(p, SIGNAL(triggered()),
							this, SLOT(jumpToLocation()));
					players.append(p);
				}
			}
		}
		jumpMenu->addActions(players);
		jumpMenu->setEnabled(hasPlayers);
		path.cdUp();
	}

	if (path.cd("data"))
	{
		loadStructures(path);
		path.cdUp();
	}

	//show dimensions
	dimensions->getDimensions(path,dimMenu,this);
	emit worldLoaded(true);
	mapview->setLocation(locations.first().x,locations.first().z);
	toggleFlags();
}

void Minutor::rescanWorlds()
{
	worlds.clear();
	getWorldList();
	worldMenu->clear();
	worldMenu->addActions(worlds);
	worldMenu->setEnabled(worlds.count()!=0);
	//we don't care about the auto-update toggle, since that only happens
	//on startup anyway.
}

void Minutor::extendStructuresMenu(QString type)
{
	if (!structuresMenuEntries.contains(type))
	{
		//generate a unique keyboard shortcut
		QKeySequence sequence;
		QString actionName;
		int ampPos = 0;
		foreach (const QChar& c, type)
		{
			sequence = QKeySequence(QString("Ctrl+")+c);
			actionName = type.mid(0, ampPos) + "&" + type.mid(ampPos);
			foreach(QMenu* m, menuBar()->findChildren<QMenu*>())
			{
				foreach (const QAction* a, m->actions())
				{
					if (a->shortcut() == sequence)
					{
						sequence = QKeySequence();
						actionName = "";
						break;
					}
				}
				if (actionName.isEmpty())
					break; //already eliminated this as a possbility
			}
			if (!actionName.isEmpty())
				break; //not eliminated, this one is ok
			++ampPos;
		}

		// append this type to list
		structuresMenuEntries.append(type);
		structuresMenu->setEnabled(true);
		structureActions.push_back(new QAction(actionName, this));
		structureActions.last()->setShortcut(sequence);
		structureActions.last()->setStatusTip(QString(tr("Toggle viewing of %1").arg(type)));
		structureActions.last()->setEnabled(true);
		structureActions.last()->setData(type);
		structureActions.last()->setCheckable(true);
		structuresMenu->addAction(structureActions.last());
		connect(structureActions.last(), SIGNAL(triggered()), this, SLOT(toggleFlags()));
	}
}

void Minutor::showProperties(int x, int y, int z)
{
/*	QMap<QString, QVariant> values;
	StructureMap::iterator it, itEnd = structures.end();
	for (it = structures.begin(); it != itEnd; ++it)
	{
		//search near the given coordinates for areas
		//TODO: alternative? bigger bins? kd tree?
		for (int ix = x - maxentitydistance; ix < x + maxentitydistance; ++ix)
		{
			for (int iz = z - maxentitydistance; iz < z + maxentitydistance; ++iz)
			{
				QList<Structure> structures = it->values(qMakePair(ix, iz));
				if (structures.size() > 0)
				{
					QList<QVariant> list;
					foreach(const Structure& e, structures)
					{
						if (e.intersects(x, 0, z, x, y+4, z))
						{
							list.push_back(e.properties);
						}
					}
					if (!list.empty())
						values.insertMulti(it.key(), list);
				}
			}
		}
	}
	if (!values.empty())
	{
		propView->DisplayProperties(values);
		propView->show();
	}*/
}

void Minutor::loadStructures(const QDir& dataPath)
{
	//attempt to parse all of the files in the data directory, looking for
	//generated structures
	foreach(const QString& fileName, dataPath.entryList(QStringList() << "*.dat"))
	{
		NBT file(dataPath.filePath(fileName));
		//no guarantee that this file contains what we expect. be careful.
		Tag* data = file.at("data");
		if (data && data != &NBT::Null)
		{
			Tag* features = data->at("Features");
			if (features && features != &NBT::Null)
			{
				//convert the features to a qvariant here
				QVariant maybeFeatureMap = features->getData();
				if ((QMetaType::Type)maybeFeatureMap.type() == QMetaType::QVariantMap)
				{
					QMap<QString, QVariant> featureMap = maybeFeatureMap.toMap();
					foreach(const QVariant& feature, featureMap)
					{
						Structure element;
						if (element.load(feature))
						{
							mapview->addStructure(element);
							extendStructuresMenu(element.getId());
						}
					}
				}
			}
		}
	}

	//parse villages.dat. it will contain information about the villages themselves
	NBT villagefile(dataPath.filePath("villages.dat"));
	Tag* data = villagefile.at("data");
	if (data && data != &NBT::Null)
	{
		Tag* villages = data->at("Villages");
		if (villages && villages != &NBT::Null)
		{
			for (int i = 0; i < villages->length(); ++i)
			{
				Tag* village = villages->at(i);
				int radius = village->at("Radius")->toInt();
				int cx = village->at("CX")->toInt();
				int cy = village->at("CY")->toInt();
				int cz = village->at("CZ")->toInt();

				Structure element( cx - radius, cy - radius, cz - radius,
								   cx + radius, cy + radius, cz + radius,
								   "Village", village->getData());
				mapview->addStructure(element);
			}
		}
	}
}
