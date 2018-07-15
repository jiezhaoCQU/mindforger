/*
 mind-navigator.cpp     MindForger thinking notebook

 Copyright (C) 2016-2018 Martin Dvorak <martin.dvorak@mindforger.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.

 ****************************************************************************
 **
 ** Copyright (C) 2016 The Qt Company Ltd.
 ** Contact: https://www.qt.io/licensing/
 **
 ** This file is part of the examples of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:BSD$
 ** Commercial License Usage
 ** Licensees holding valid commercial Qt licenses may use this file in
 ** accordance with the commercial license agreement provided with the
 ** Software or, alternatively, in accordance with the terms contained in
 ** a written agreement between you and The Qt Company. For licensing terms
 ** and conditions see https://www.qt.io/terms-conditions. For further
 ** information use the contact form at https://www.qt.io/contact-us.
 **
 ** BSD License Usage
 ** Alternatively, you may use this file under the terms of the BSD license
 ** as follows:
 **
 ** "Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions are
 ** met:
 **   * Redistributions of source code must retain the above copyright
 **     notice, this list of conditions and the following disclaimer.
 **   * Redistributions in binary form must reproduce the above copyright
 **     notice, this list of conditions and the following disclaimer in
 **     the documentation and/or other materials provided with the
 **     distribution.
 **   * Neither the name of The Qt Company Ltd nor the names of its
 **     contributors may be used to endorse or promote products derived
 **     from this software without specific prior written permission.
 **
 **
 ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 ** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 ** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 ** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 ** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 ** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 ** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 ** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 ** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 ** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 ** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 **
 ** $QT_END_LICENSE$
 **
 ****************************************************************************/

#include "navigator_view.h"
#include "edge.h"
#include "node.h"

#include <math.h>

#include <QKeyEvent>

namespace m8r {

using namespace std;

NavigatorView::NavigatorView(QWidget* parent)
    : QGraphicsView(parent),
      timerId{0},
      w{},
      h{},
      garbageItems{},
      subgraph{},
      doShowLegend{true}
{
    // scene is peephole rectangle to the whole view (QGraphicsView)
    navigatorScene = new QGraphicsScene(this);
    navigatorScene->setItemIndexMethod(QGraphicsScene::NoIndex);
    setScene(navigatorScene);

	setCacheMode(CacheBackground);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	setRenderHint(QPainter::Antialiasing);
	setTransformationAnchor(AnchorUnderMouse);
}

NavigatorView::~NavigatorView()
{
    navigatorScene->clear();
    clearGarbageItems();
}

void NavigatorView::clearGarbageItems()
{
    for(QGraphicsItem* i:garbageItems) delete i;
    garbageItems.clear();
}

void NavigatorView::cleanupBeforeHide() {
    lock_guard<mutex> criticalSection{refreshMutex};

    // TODO codereview to ensure that there are no memory leaks
    clearGarbageItems();
    navigatorScene->clear();

    MF_DEBUG("NAVIGATOR.hide() scene[" << navigatorScene->items().size() << "]" << endl);
}

void NavigatorView::updateNavigatorView()
{
    // IMPROVE: resize scene also on window resize event
    w = static_cast<QWidget*>(parent())->width();
    h = static_cast<QWidget*>(parent())->height();
    // the first two args specify location of the [0,0]
    navigatorScene->setSceneRect(-w/4, -h/4, w, h);
    // scale scene to avoid scroll bars
    scale(qreal(0.99), qreal(0.99));
    //setMinimumSize(WIDTH, HEIGHT);
}

void NavigatorView::itemMoved()
{
    if(!timerId) {
        // IMPROVE dynamic refresh rate
        timerId = startTimer(1000/25);
    }
}

void NavigatorView::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {

    // IMPROVE choose last selected node and move that selected node
//	case Qt::Key_Up:
//        mindNode->moveBy(0, -20);
//		break;
//	case Qt::Key_Down:
//        mindNode->moveBy(0, 20);
//		break;
//	case Qt::Key_Left:
//        mindNode->moveBy(-20, 0);
//		break;
//	case Qt::Key_Right:
//        mindNode->moveBy(20, 0);
//		break;

	case Qt::Key_Plus:
		zoomIn();
		break;
	case Qt::Key_Minus:
		zoomOut();
		break;
	case Qt::Key_Space:
	case Qt::Key_Enter:
		shuffle();
		break;
	default:
		QGraphicsView::keyPressEvent(event);
	}
}

void NavigatorView::timerEvent(QTimerEvent *event)
{
    lock_guard<mutex> criticalSection{refreshMutex};
    Q_UNUSED(event);

    // if subgraph != nullptr then it's a request to draw new scene
    if(subgraph) {

        //  REMOVE old nodes

        NavigatorNode* selectedNode{};
        // blank scene > the first (central) node must be created
        if(navigatorScene->items().isEmpty()) {
            if(subgraph->getCentralNode()) {
                MF_DEBUG("  NEW view - CREATING central node: " << subgraph->getCentralNode()->getName() << endl);
                // empty scene ~ it's save to clear garbage
                clearGarbageItems();

                selectedNode = new NavigatorNode(subgraph->getCentralNode(), this, subgraph->getCentralNode()->getColor());
                navigatorScene->addItem(selectedNode);
                selectedNode->setPos(0, 0);
            } else {
                MF_DEBUG("  sub-graph is EMPTY");
                navigatorScene->clear();
                return;
            }
        } else {
            if(subgraph->getCentralNode()) {
                MF_DEBUG("  KILL all except selected node: " << subgraph->getCentralNode()->getName() << endl);
                MF_DEBUG("  BEFORE scene[" << navigatorScene->items().size() << "]" << endl);
                for(QGraphicsItem* item:navigatorScene->items()) {
                    MF_DEBUG("   FDG.destroy()[" << item->childItems().size() << "]" << endl);

                    NavigatorNode* n = qgraphicsitem_cast<NavigatorNode*>(item);
                    if(n) {
                        // kill node
                        if(n->getKnowledgeGraphNode() != subgraph->getCentralNode()) {
                            MF_DEBUG("    NODE " << n->getName().toStdString() << endl);
                            navigatorScene->removeItem(n);

                            // mouse events CANNOT be stopped (events go to item being removed) > deleted on navigator view HIDE
                            garbageItems.push_back(item);
                        } else {
                            MF_DEBUG("    SAFE: " << subgraph->getCentralNode()->getName() << endl);
                            selectedNode = n;
                        }
                    } else {
                        MF_DEBUG("    EDGE " << endl);
                        // kill edge
                        navigatorScene->removeItem(item);

                        // mouse events CANNOT be stopped (events go to item being removed) > deleted on navigator view HIDE
                        garbageItems.push_back(item);
                    }
                }
                MF_DEBUG("  AFTER scene[" << navigatorScene->items().size() << "]" << endl);
            } else {
                MF_DEBUG("  sub-graph is EMPTY");
                navigatorScene->clear();
                return;
            }
        }

        //  ADD new nodes

        NavigatorNode* n;
        NavigatorEdge* e;
        std::vector<KnowledgeGraphNode*>& children = subgraph->getChildren();
        MF_DEBUG("  ADD children[" << children.size() << "]" << endl);
        // TODO QColor(t->getColor().asLong())
        for(KnowledgeGraphNode* kn:children) {
            // newly created nodes and edges will be destroyed by garbage items collector
            n = new NavigatorNode(kn, this, QColor(kn->getColor()));
            navigatorScene->addItem(n);
            e = new NavigatorEdge(selectedNode, n);
            navigatorScene->addItem(e);
            n->setPos(qrand()%w, qrand()%h);
        }

        std::vector<KnowledgeGraphNode*>& parents = subgraph->getParents();
        MF_DEBUG("  ADD parents[" << parents.size() << "]" << endl);
        for(KnowledgeGraphNode* kn:parents) {
            // newly created nodes and edges will be destroyed by garbage items collector
            n = new NavigatorNode(kn, this, QColor(kn->getColor()));
            navigatorScene->addItem(n);
            e = new NavigatorEdge(n, selectedNode);
            navigatorScene->addItem(e);
            n->setPos(qrand()%w, qrand()%h);
        }

        subgraph = nullptr;

        MF_DEBUG("  DONE scene[" << navigatorScene->items().size() << "]" << endl);
    }

    // RENDER scene

    QList<NavigatorNode*> nodes;
    foreach(QGraphicsItem *item, navigatorScene->items()) {
        if(NavigatorNode *node = qgraphicsitem_cast<NavigatorNode *>(item))
            nodes << node;
    }

    foreach(NavigatorNode *node, nodes) {
        node->calculateForces();
    }

    bool itemsMoved = false;
    foreach(NavigatorNode *node, nodes) {
        if(node->advance()) {
            itemsMoved = true;
        }
    }

    if(!itemsMoved) {
        killTimer(timerId);
        timerId = 0;
    }
}

#ifndef QT_NO_WHEELEVENT
void NavigatorView::wheelEvent(QWheelEvent *event)
{
    scaleView(pow((double)2, -event->delta() / 240.0));
}
#endif

void NavigatorView::drawBackground(QPainter *painter, const QRectF &rect)
{
    Q_UNUSED(rect);

    // TODO configurable from configuration, setting refreshed to local field on refresh() invocation (performance)
    if(doShowLegend) {
        QRectF sceneRect = this->sceneRect();
        QRectF textRect(sceneRect.left() + 4, sceneRect.top() + 4, sceneRect.width() - 4, sceneRect.height() - 4);
        QString message(tr("Click and drag the nodes around, and zoom with the mouse wheel or the '+' and '-' keys"));

        QFont font = painter->font();
        font.setBold(true);
        font.setPointSize(10);
        painter->setFont(font);
        painter->setPen(Qt::darkGray);
        painter->drawText(textRect, message);
    }
}

void NavigatorView::scaleView(qreal scaleFactor)
{
    qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < 0.07 || factor > 100)
        return;

	scale(scaleFactor, scaleFactor);
}

void NavigatorView::shuffle()
{
    foreach (QGraphicsItem *item, navigatorScene->items()) {
        if (qgraphicsitem_cast<NavigatorNode *>(item))
			item->setPos(-150 + qrand() % 300, -150 + qrand() % 300);
	}
}

void NavigatorView::zoomIn()
{
    scaleView(qreal(1.2));
}

void NavigatorView::zoomOut()
{
    scaleView(1 / qreal(1.2));
}

/*
 * ... refresh on a specific node selection ...
 */

void NavigatorView::iWasSelected(NavigatorNode* selectedNode)
{
    emit nodeSelectedSignal(selectedNode);
}

}  // m8r namespace
