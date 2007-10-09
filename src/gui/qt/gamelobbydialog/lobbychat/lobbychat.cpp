/***************************************************************************
 *   Copyright (C) 2006 by FThauer FHammer   *
 *   f.thauer@web.de   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "lobbychat.h"
#define IN_INCLUDE_LIBIRC_H
#include <core/libircclient/include/libirc_rfcnumeric.h>
#include <net/socket_msg.h>

#include "gamelobbydialogimpl.h"
#include "session.h"
#include "configfile.h"

using namespace std;


LobbyChat::LobbyChat(gameLobbyDialogImpl* l, ConfigFile *c) : myLobby(l), myConfig(c)
{

	chatInputCompleter = new QCompleter(myLobby->treeWidget_NickList->model());
	chatInputCompleter->setCaseSensitivity(Qt::CaseInsensitive);
	chatInputCompleter->setCompletionMode(QCompleter::PopupCompletion);
 	myLobby->lineEdit_ChatInput->setCompleter(chatInputCompleter);

}

LobbyChat::~LobbyChat()
{

}

void LobbyChat::sendMessage() {

	if (myNick.size())
	{
		fillChatLinesHistory(myLobby->lineEdit_ChatInput->text());
		QString tmpMsg(myLobby->lineEdit_ChatInput->text());
		if (tmpMsg.size())
		{
			myLobby->getSession().sendIrcChatMessage(tmpMsg.toUtf8().constData());
			myLobby->lineEdit_ChatInput->setText("");

			displayMessage(myNick, tmpMsg);
		}
	}
}

void LobbyChat::connected(QString server)
{
	myLobby->textBrowser_ChatDisplay->append(tr("Successfully connected to") + " " + server + ".");
}

void LobbyChat::selfJoined(QString ownName, QString channel)
{
	myNick = ownName;
	myLobby->textBrowser_ChatDisplay->append(tr("Joined channel:") + " " + channel + " " + tr("as user") + " " + ownName + ".");
	myLobby->textBrowser_ChatDisplay->append("");
	myLobby->lineEdit_ChatInput->setEnabled(true);
	myLobby->lineEdit_ChatInput->setFocus();
}

void LobbyChat::playerJoined(QString playerName)
{
	QTreeWidgetItem *item = new QTreeWidgetItem(myLobby->treeWidget_NickList, 0);
	item->setData(0, Qt::DisplayRole, playerName);

	myLobby->treeWidget_NickList->sortItems(0, Qt::AscendingOrder);
}

void LobbyChat::playerChanged(QString oldNick, QString newNick)
{
	QList<QTreeWidgetItem *> tmpList(myLobby->treeWidget_NickList->findItems(oldNick, Qt::MatchExactly));
	if (!tmpList.empty())
		tmpList.front()->setData(0, Qt::DisplayRole, newNick);
	else
	{
		tmpList = myLobby->treeWidget_NickList->findItems("@" + oldNick, Qt::MatchExactly);
		if (!tmpList.empty())
			tmpList.front()->setData(0, Qt::DisplayRole, "@" + newNick);
	}

	if (myNick == oldNick)
		myNick = newNick;
}

void LobbyChat::playerKicked(QString nickName, QString byWhom, QString reason)
{
	if (myNick == nickName)
	{
		myLobby->accept();
		QMessageBox::warning(myLobby, tr("Network Notification"),
			tr("You were kicked from the server."),
				QMessageBox::Close);
	}
	else
		myLobby->textBrowser_ChatDisplay->append(nickName + " " + tr("was kicked from the server by") + " " + byWhom + " (" + reason + ")");
}

void LobbyChat::playerLeft(QString playerName)
{
	QList<QTreeWidgetItem *> tmpList(myLobby->treeWidget_NickList->findItems(playerName, Qt::MatchExactly));
	if (!tmpList.empty())
		myLobby->treeWidget_NickList->takeTopLevelItem(myLobby->treeWidget_NickList->indexOfTopLevelItem(tmpList.front()));

	myLobby->treeWidget_NickList->sortItems(0, Qt::AscendingOrder);
}

void LobbyChat::displayMessage(QString playerName, QString message) { 
	
	QString tempMsg;
	if(message.contains(QString::fromUtf8(myConfig->readConfigString("MyName").c_str()), Qt::CaseInsensitive)) {
		tempMsg = QString("<b>"+message+"</b>");
		if(myLobby->isVisible() && myConfig->readConfigInt("PlayLobbyChatNotification")) 
			myLobby->getMyW()->getMySDLPlayer()->playSound("lobbychatnotify",0);
	}
	else {
		tempMsg = message;
	}
	myLobby->textBrowser_ChatDisplay->append(playerName + ": " + tempMsg); 
}

void LobbyChat::checkInputLength(QString string) {

	 if(string.toUtf8().length() > 120) myLobby->lineEdit_ChatInput->setMaxLength(string.length());  
}

void LobbyChat::clearChat() {

	myNick = "";
	myLobby->treeWidget_NickList->clear();
	myLobby->textBrowser_ChatDisplay->clear();
	myLobby->textBrowser_ChatDisplay->append(tr("Connecting to Chat server..."));
	myLobby->lineEdit_ChatInput->setEnabled(false);
/*	QStringList wordList;
	wordList << "alpha" << "omega" << "omicron" << "zeta";*/
	
// 	QCompleter *completer = new QCompleter(wordList, this);
// 	completer->setCaseSensitivity(Qt::CaseInsensitive);
// 	completer->setCompletionMode(QCompleter::InlineCompletion);
// // 	lineEdit_ChatInput->setCompleter(completer);
	
}

void LobbyChat::chatError(int errorCode)
{
	QString errorMsg;
	switch (errorCode)
	{
		case ERR_IRC_INTERNAL :
		case ERR_IRC_SELECT_FAILED :
		case ERR_IRC_RECV_FAILED :
		case ERR_IRC_SEND_FAILED :
		case ERR_IRC_INVALID_PARAM :
			errorMsg = tr("An internal IRC error has occured:") + " " + QString::number(errorCode);
			break;
		case ERR_IRC_CONNECT_FAILED :
			errorMsg = tr("Could not connect to the IRC server. Chat will be unavailable.");
			break;
		case ERR_IRC_TERMINATED :
			errorMsg = tr("The IRC server terminated the connection. Chat will be unavailable.");
			break;
		case ERR_IRC_TIMEOUT :
			errorMsg = tr("An IRC action timed out.");
			break;
	}
	if (errorMsg.size())
		myLobby->textBrowser_ChatDisplay->append(errorMsg); 
}

void LobbyChat::chatServerError(int errorCode)
{
	QString errorMsg;
	switch (errorCode)
	{
		case LIBIRC_RFC_ERR_NOSUCHNICK :
		case LIBIRC_RFC_ERR_NOSUCHCHANNEL :
		case LIBIRC_RFC_ERR_CANNOTSENDTOCHAN :
		case LIBIRC_RFC_ERR_TOOMANYCHANNELS :
		case LIBIRC_RFC_ERR_WASNOSUCHNICK :
		case LIBIRC_RFC_ERR_TOOMANYTARGETS :
		case LIBIRC_RFC_ERR_NOSUCHSERVICE :
		case LIBIRC_RFC_ERR_NOORIGIN :
		case LIBIRC_RFC_ERR_NORECIPIENT :
		case LIBIRC_RFC_ERR_NOTEXTTOSEND :
		case LIBIRC_RFC_ERR_NOTOPLEVEL :
		case LIBIRC_RFC_ERR_WILDTOPLEVEL :
		case LIBIRC_RFC_ERR_BADMASK :
		case LIBIRC_RFC_ERR_UNKNOWNCOMMAND :
		case LIBIRC_RFC_ERR_NOMOTD :
		case LIBIRC_RFC_ERR_NOADMININFO :
		case LIBIRC_RFC_ERR_FILEERROR :
		case LIBIRC_RFC_ERR_NONICKNAMEGIVEN :
		case LIBIRC_RFC_ERR_ERRONEUSNICKNAME :
		case LIBIRC_RFC_ERR_UNAVAILRESOURCE :
		case LIBIRC_RFC_ERR_USERNOTINCHANNEL :
		case LIBIRC_RFC_ERR_NOTONCHANNEL :
		case LIBIRC_RFC_ERR_USERONCHANNEL :
		case LIBIRC_RFC_ERR_NOLOGIN :
		case LIBIRC_RFC_ERR_SUMMONDISABLED :
		case LIBIRC_RFC_ERR_USERSDISABLED :
		case LIBIRC_RFC_ERR_NOTREGISTERED :
		case LIBIRC_RFC_ERR_NEEDMOREPARAMS :
		case LIBIRC_RFC_ERR_ALREADYREGISTRED :
		case LIBIRC_RFC_ERR_NOPERMFORHOST :
		case LIBIRC_RFC_ERR_PASSWDMISMATCH :
		case LIBIRC_RFC_ERR_YOUREBANNEDCREEP :
		case LIBIRC_RFC_ERR_YOUWILLBEBANNED :
		case LIBIRC_RFC_ERR_KEYSET :
		case LIBIRC_RFC_ERR_CHANNELISFULL :
		case LIBIRC_RFC_ERR_UNKNOWNMODE :
		case LIBIRC_RFC_ERR_INVITEONLYCHAN :
		case LIBIRC_RFC_ERR_BANNEDFROMCHAN :
		case LIBIRC_RFC_ERR_BADCHANNELKEY :
		case LIBIRC_RFC_ERR_BADCHANMASK :
		case LIBIRC_RFC_ERR_NOCHANMODES :
		case LIBIRC_RFC_ERR_BANLISTFULL :
		case LIBIRC_RFC_ERR_NOPRIVILEGES :
		case LIBIRC_RFC_ERR_CHANOPRIVSNEEDED :
		case LIBIRC_RFC_ERR_CANTKILLSERVER :
		case LIBIRC_RFC_ERR_RESTRICTED :
		case LIBIRC_RFC_ERR_UNIQOPPRIVSNEEDED :
		case LIBIRC_RFC_ERR_NOOPERHOST :
		case LIBIRC_RFC_ERR_UMODEUNKNOWNFLAG :
		case LIBIRC_RFC_ERR_USERSDONTMATCH :
			errorMsg = tr("The IRC server reported an error:") + " " + QString::number(errorCode);
			break;
	}
//	if (errorMsg.size())
//		myLobby->textBrowser_ChatDisplay->append(errorMsg); 
}

void LobbyChat::fillChatLinesHistory(QString fillString) {

	chatLinesHistory << fillString;
	if(chatLinesHistory.size() > 50) chatLinesHistory.removeFirst();

// 	QStringList::const_iterator constIterator;
//      	for (constIterator = chatLinesHistory.constBegin(); constIterator != chatLinesHistory.constEnd(); ++constIterator) {
//          	cout << (*constIterator).toLocal8Bit().constData() << endl;
// 	}
}

void LobbyChat::showChatHistoryIndex(int index) { 

	if(index <= chatLinesHistory.size() && index > 0) {

// 		cout << chatLinesHistory.size() << " : " <<  index << endl;
		myLobby->lineEdit_ChatInput->setText(chatLinesHistory.at(chatLinesHistory.size()-(index)));  
	}
}
